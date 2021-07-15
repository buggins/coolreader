#ifndef __LVINKMEASUREMENTDRAWBUF_H_INCLUDED__
#define __LVINKMEASUREMENTDRAWBUF_H_INCLUDED__

#include "lvbasedrawbuf.h"

/// Ink measurement buffer
class LVInkMeasurementDrawBuf : public LVBaseDrawBuf
{
private:
    int ink_top_y;
    int ink_bottom_y;
    int ink_left_x;
    int ink_right_x;
    bool has_ink;
    bool measure_hidden_content;
    bool ignore_decorations; // ignore borders and background
public:
    /// get buffer bits per pixel
    virtual int  GetBitsPerPixel() const { return 8; }

    /// wants to be fed hidden content (only LVInkMeasurementDrawBuf may return true)
    virtual bool WantsHiddenContent() const { return measure_hidden_content; }

    /// fills buffer with specified color
    virtual void Clear( lUInt32 color ) {
        has_ink = false;
    }

    /// fills rectangle with specified color
    void updateInkBounds( int x0, int y0, int x1, int y1 );

    bool getInkArea( lvRect &rect );

    /// fills rectangle with specified color
    virtual void FillRect( int x0, int y0, int x1, int y1, lUInt32 color );
    virtual void FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 * pattern );
    /// draws image
    virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither );
    /// blend font bitmap using specified palette
    virtual void BlendBitmap( int x, int y, const lUInt8 * bitmap, FontBmpPixelFormat bitmap_fmt, int width, int height, int bmp_pitch, lUInt32 * palette );
    /// draw line
    virtual void DrawLine( int x0, int y0, int x1, int y1, lUInt32 color0, int length1=1, int length2=0, int direction=0 );

    virtual void GetClipRect( lvRect * clipRect ) const;

    /// create own draw buffer
    explicit LVInkMeasurementDrawBuf( bool measurehiddencontent=false, bool ignoredecorations=false)
        : ink_top_y(0), ink_bottom_y(0), ink_left_x(0), ink_right_x(0) , has_ink(false)
        , measure_hidden_content(measurehiddencontent) , ignore_decorations(ignoredecorations)
        {}
    /// destructor
    virtual ~LVInkMeasurementDrawBuf() {}

    // Unused methods in the context of lvrend that we need to have defined
    virtual void Rotate( cr_rotate_angle_t angle ) {}
    virtual lUInt32 GetWhiteColor() const { return 0; }
    virtual lUInt32 GetBlackColor() const { return 0; }
    virtual void DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette ) {}
    virtual void DrawOnTop( LVDrawBuf * __restrict buf, int x, int y) {}
    virtual void DrawRescaled(LVDrawBuf * src, int x, int y, int dx, int dy, int options) {}
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
    virtual void DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette ) {}
#endif
    virtual void Invert() {}
    virtual lUInt32 GetPixel( int x, int y ) const { return 0; }
    virtual void InvertRect( int x0, int y0, int x1, int y1 ) {}
    virtual void Resize( int dx, int dy ) {}
    virtual lUInt8 * GetScanLine( int y ) const { return 0; }
};

#endif  // __LVINKMEASUREMENTDRAWBUF_H_INCLUDED__
