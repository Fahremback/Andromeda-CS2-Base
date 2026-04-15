#include "Math.hpp"

#include <cfloat>
#include <cmath>
#include <immintrin.h>
#include <ImGui/imgui_internal.h>

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>

namespace Math
{
    auto WorldToScreen( const Vector3& vIn , ImVec2& vOut ) -> bool
    {
        auto ret = false;

        Vector3 Out;

        ret = !( ScreenTransform( vIn , Out ) );

        auto ScreenWidth = 0;
        auto ScreenHeight = 0;

        SDK::Interfaces::EngineToClient()->GetScreenSize( ScreenWidth , ScreenHeight );

        vOut.x = ( ( Out.m_x + 1.0f ) * 0.5f ) * static_cast<float>( ScreenWidth );
        vOut.y = static_cast<float>( ScreenHeight ) - ( ( ( Out.m_y + 1.0f ) * 0.5f ) * static_cast<float>( ScreenHeight ) );

        return ret;
    }

    auto WorldToScreen( const Vector3& vIn , Vector2& vOut ) -> bool
    {
        auto ret = false; 

        Vector3 Out;

        ret = !( ScreenTransform( vIn , Out ) );

        auto ScreenWidth = 0;
        auto ScreenHeight = 0;

        SDK::Interfaces::EngineToClient()->GetScreenSize( ScreenWidth , ScreenHeight );

        vOut.m_x = ( ( Out.m_x + 1.0f ) * 0.5f ) * static_cast<float>( ScreenWidth );
        vOut.m_y = static_cast<float>( ScreenHeight ) - ( ( ( Out.m_y + 1.0f ) * 0.5f ) * static_cast<float>( ScreenHeight ) );

        return ret;
    }

    auto WorldToScreen( const Vector3& vIn , Vector3& vOut ) -> bool
    {
        auto ret = false;
        
        ret = !( ScreenTransform( vIn , vOut ) );

        auto ScreenWidth = 0;
        auto ScreenHeight = 0;

        SDK::Interfaces::EngineToClient()->GetScreenSize( ScreenWidth , ScreenHeight );

        vOut.m_x = ( ( vOut.m_x + 1.0f ) * 0.5f ) * static_cast<float>( ScreenWidth );
        vOut.m_y = static_cast<float>( ScreenHeight ) - ( ( ( vOut.m_y + 1.0f ) * 0.5f ) * static_cast<float>( ScreenHeight ) );

        return ret;
    }

    auto WorldToScreenBatch( const Vector3* pIn , ImVec2* pOut , bool* pVisible , size_t Count ) -> size_t
    {
        if ( !pIn || !pOut || !pVisible || !Count )
            return 0;

        auto ScreenWidth = 0;
        auto ScreenHeight = 0;
        SDK::Interfaces::EngineToClient()->GetScreenSize( ScreenWidth , ScreenHeight );

        const float Width = static_cast<float>( ScreenWidth );
        const float Height = static_cast<float>( ScreenHeight );

        size_t VisibleCount = 0;
        size_t Index = 0;

#if defined( __AVX__ )
        constexpr size_t SIMD_WIDTH = 8;
        alignas( 64 ) float ClipX[SIMD_WIDTH]{};
        alignas( 64 ) float ClipY[SIMD_WIDTH]{};

        const __m256 One = _mm256_set1_ps( 1.0f );
        const __m256 Half = _mm256_set1_ps( 0.5f );
        const __m256 WidthVec = _mm256_set1_ps( Width );
        const __m256 HeightVec = _mm256_set1_ps( Height );

        for ( ; Index + SIMD_WIDTH <= Count; Index += SIMD_WIDTH )
        {
            for ( size_t i = 0; i < SIMD_WIDTH; ++i )
            {
                Vector3 Projected{};
                const bool IsVisible = !ScreenTransform( pIn[Index + i] , Projected );
                pVisible[Index + i] = IsVisible;

                ClipX[i] = Projected.m_x;
                ClipY[i] = Projected.m_y;
            }

            const __m256 X = _mm256_load_ps( ClipX );
            const __m256 Y = _mm256_load_ps( ClipY );

            const __m256 XNorm = _mm256_mul_ps( _mm256_add_ps( X , One ) , Half );
            const __m256 YNorm = _mm256_mul_ps( _mm256_add_ps( Y , One ) , Half );

#if defined( __FMA__ )
            const __m256 ScreenX = _mm256_fmadd_ps( XNorm , WidthVec , _mm256_setzero_ps() );
            const __m256 ScreenYNorm = _mm256_fmadd_ps( YNorm , HeightVec , _mm256_setzero_ps() );
#else
            const __m256 ScreenX = _mm256_mul_ps( XNorm , WidthVec );
            const __m256 ScreenYNorm = _mm256_mul_ps( YNorm , HeightVec );
#endif
            const __m256 ScreenY = _mm256_sub_ps( HeightVec , ScreenYNorm );

            _mm256_store_ps( ClipX , ScreenX );
            _mm256_store_ps( ClipY , ScreenY );

            for ( size_t i = 0; i < SIMD_WIDTH; ++i )
            {
                pOut[Index + i].x = ClipX[i];
                pOut[Index + i].y = ClipY[i];

                if ( pVisible[Index + i] )
                    ++VisibleCount;
            }
        }
#endif

        for ( ; Index < Count; ++Index )
        {
            Vector3 Projected{};
            const bool IsVisible = !ScreenTransform( pIn[Index] , Projected );

            pVisible[Index] = IsVisible;
            pOut[Index].x = ( ( Projected.m_x + 1.0f ) * 0.5f ) * Width;
            pOut[Index].y = Height - ( ( ( Projected.m_y + 1.0f ) * 0.5f ) * Height );

            if ( IsVisible )
                ++VisibleCount;
        }

        return VisibleCount;
    }

    auto NormalizeVectorFast( Vector3& Vec ) -> void
    {
        const float LengthSq = Vec.m_x * Vec.m_x + Vec.m_y * Vec.m_y + Vec.m_z * Vec.m_z;

        if ( LengthSq <= FLT_EPSILON )
            return;

#if defined( __AVX512F__ )
        const __m128 LengthSqVec = _mm_set_ss( LengthSq );
        const __m128 InvLenVec = _mm_rsqrt14_ss( LengthSqVec );
        const float InvLen = _mm_cvtss_f32( InvLenVec );
#else
        const float InvLen = 1.0f / std::sqrt( LengthSq );
#endif

        Vec.m_x *= InvLen;
        Vec.m_y *= InvLen;
        Vec.m_z *= InvLen;
    }

    auto AngleNormalize( float angle ) -> float
    {
        angle = std::fmod( angle , 360.f );

        if ( angle > 180.f )
            angle -= 360.f;
        else if ( angle < -180.f )
            angle += 360.f;

        return angle;
    }

    auto NormalizeAngles( QAngle& QAngle ) -> void
    {
        for ( auto i = 0; i < 3; i++ )
        {
            while ( QAngle[i] < -180.0f ) QAngle[i] += 360.0f;
            while ( QAngle[i] > 180.0f ) QAngle[i] -= 360.0f;
        }
    }

    auto ClampAngles( QAngle& QAngle ) -> void
    {
        if ( QAngle.m_x > 89.0f ) QAngle.m_x = 89.0f;
        else if ( QAngle.m_x < -89.0f ) QAngle.m_x = -89.0f;

        if ( QAngle.m_y > 180.0f ) QAngle.m_y = 180.0f;
        else if ( QAngle.m_y < -180.0f ) QAngle.m_y = -180.0f;

        QAngle.m_z = 0;
    }

    auto CalcAngle( const Vector3& src , const Vector3& dst ) -> QAngle
    {
        QAngle vAngle;

        Vector3 delta( ( src.m_x - dst.m_x ) , ( src.m_y - dst.m_y ) , ( src.m_z - dst.m_z ) );
        double hyp = sqrt( delta.m_x * delta.m_x + delta.m_y * delta.m_y );

        vAngle.m_x = float( atanf( float( delta.m_z / hyp ) ) * 57.295779513082f );
        vAngle.m_y = float( atanf( float( delta.m_y / delta.m_x ) ) * 57.295779513082f );
        vAngle.m_z = 0.0f;

        if ( delta.m_x >= 0.0 )
            vAngle.m_y += 180.0f;

        return vAngle;
    }

    auto VectorTransform( const Vector3& vIn1 , matrix3x4_t& vIn2 , Vector3& vOut ) -> void
    {
        vOut.m_x = vIn1.m_x * vIn2[0][0] + vIn1.m_y * vIn2[0][1] + vIn1.m_z * vIn2[0][2] + vIn2[0][3];
        vOut.m_y = vIn1.m_x * vIn2[1][0] + vIn1.m_y * vIn2[1][1] + vIn1.m_z * vIn2[1][2] + vIn2[1][3];
        vOut.m_z = vIn1.m_x * vIn2[2][0] + vIn1.m_y * vIn2[2][1] + vIn1.m_z * vIn2[2][2] + vIn2[2][3];
    }

    auto AngleVectors( const QAngle& QAngle , Vector3& vForward ) -> void
    {
        vec_t sp , sy , cp , cy;

        sp = sinf( DEG2RAD( QAngle[0] ) );
        cp = cosf( DEG2RAD( QAngle[0] ) );

        sy = sinf( DEG2RAD( QAngle[1] ) );
        cy = cosf( DEG2RAD( QAngle[1] ) );

        vForward.m_x = cp * cy;
        vForward.m_y = cp * sy;
        vForward.m_z = -sp;
    }

    auto AngleVectors( const QAngle& QAngle , Vector3& vForward , Vector3& vRight , Vector3& vUp ) -> void
    {
        vec_t sr , sp , sy , cr , cp , cy;

        sp = sinf( DEG2RAD( QAngle[0] ) );
        cp = cosf( DEG2RAD( QAngle[0] ) );

        sy = sinf( DEG2RAD( QAngle[1] ) );
        cy = cosf( DEG2RAD( QAngle[1] ) );

        sr = sinf( DEG2RAD( QAngle[2] ) );
        cr = cosf( DEG2RAD( QAngle[2] ) );

        vForward.m_x = ( cp * cy );
        vForward.m_y = ( cp * sy );
        vForward.m_z = ( -sp );

        vRight.m_x = ( -1 * sr * sp * cy + -1 * cr * -sy );
        vRight.m_y = ( -1 * sr * sp * sy + -1 * cr * cy );
        vRight.m_z = ( -1 * sr * cp );

        vUp.m_x = ( cr * sp * cy + -sr * -sy );
        vUp.m_y = ( cr * sp * sy + -sr * cy );
        vUp.m_z = ( cr * cp );
    }

    auto VectorAngles( const Vector3& vForward , QAngle& QAngle ) -> void
    {
        float tmp , yaw , pitch;

        if ( vForward[1] == 0.f && vForward[0] == 0.f )
        {
            yaw = 0.f;

            if ( vForward[2] > 0 )
                pitch = 270.f;
            else
                pitch = 90.f;
        }
        else
        {
            yaw = ( atan2f( vForward[1] , vForward[0] ) * 180.f / M_PI_F );

            if ( yaw < 0 )
                yaw += 360.f;

            tmp = sqrtf( vForward[0] * vForward[0] + vForward[1] * vForward[1] );
            pitch = ( atan2f( -vForward[2] , tmp ) * 180.f / M_PI_F );

            if ( pitch < 0 )
                pitch += 360.f;
        }

        QAngle[0] = pitch;
        QAngle[1] = yaw;
        QAngle[2] = 0;
    }

    auto SmoothAngles( QAngle QViewAngles , QAngle QAimAngles , QAngle& QOutAngles , float Smoothing ) -> void
    {
        if ( Smoothing < 1.f )
            Smoothing = 1.f;

        QAngle qDiffAngles = QAimAngles - QViewAngles;

        NormalizeAngles( qDiffAngles );
        ClampAngles( qDiffAngles );

        QOutAngles = qDiffAngles / Smoothing + QViewAngles;

        NormalizeAngles( QOutAngles );
        ClampAngles( QOutAngles );
    }

    auto RotateTriangle( std::array<Vector2 , 3>& points , float rotation ) -> void
    {
        const auto points_center = ( points.at( 0 ) + points.at( 1 ) + points.at( 2 ) ) / 3;
        
        for ( auto& point : points )
        {
            point -= points_center;

            const auto temp_x = point.m_x;
            const auto temp_y = point.m_y;

            const auto theta = DEG2RAD( rotation );
            const auto c = cosf( theta );
            const auto s = sinf( theta );

            point.m_x = temp_x * c - temp_y * s;
            point.m_y = temp_x * s + temp_y * c;

            point += points_center;
        }
    }

    auto CalculateCameraPosition( Vector3 anchorPos , float distance , QAngle viewAngles ) -> Vector3
    {
        float yaw = DEG2RAD( viewAngles.m_y );
        float pitch = DEG2RAD( viewAngles.m_x );

        float x = anchorPos.m_x + distance * cosf( yaw ) * cosf( pitch );
        float y = anchorPos.m_y + distance * sinf( yaw ) * cosf( pitch );
        float z = anchorPos.m_z + distance * sinf( pitch );

        return { x, y, z };
    }

    auto AnglesToPixels( QAngle SourceAngles , QAngle FinalAngles , float Sensitivity , float Pitch , float Yaw ) -> Vector3
    {
        QAngle delta = SourceAngles - FinalAngles;

        delta.Normalize();

        float xMove = ( -delta.m_x ) / ( Yaw * Sensitivity );
        float yMove = ( delta.m_y ) / ( Pitch * Sensitivity );

        return Vector3( yMove , xMove , 0.0f );
    }

    auto PixelsDeltaToAnglesDelta( Vector3 PixelsDelta , float Sensitivity , float Pitch , float Yaw ) -> QAngle
    {
        float xMove = ( -PixelsDelta.m_x ) * ( Yaw * Sensitivity );
        float yMove = ( PixelsDelta.m_y ) * ( Pitch * Sensitivity );

        return QAngle( yMove , xMove , 0.0f );
    }
}
