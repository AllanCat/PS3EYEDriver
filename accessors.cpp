#include "ps3eye.hpp"
#include "mgr.hpp"

using ps3eye::detail::usb_manager;
using ps3eye::detail::_ps3eye_debug_status;
using ps3eye::detail::ps3eye_debug;

namespace ps3eye {

void camera::set_auto_gain(bool val)
{
    auto_gain_ = val;
    if (val)
    {
        sccb_reg_write(0x13, sccb_reg_read(0x13) | 0x04); // AGC enable
        sccb_reg_write(0x64, sccb_reg_read(0x64) | 0x03); // Gamma function ON/OFF selection
    }
    else
    {
		sccb_reg_write(0x13, sccb_reg_read(0x13) & ~0x04);
        sccb_reg_write(0x64, sccb_reg_read(0x64) & ~0x03);
    }
}

void camera::set_awb(bool val)
{
    awb_ = val;

    if (val)
    {
        sccb_reg_write(0x13, sccb_reg_read(0x13) | 0x02); // AWB enable
		sccb_reg_write(0x63, sccb_reg_read(0x63) | 0x40); // AWB calculate enable
    }
    else
    {
        sccb_reg_write(0x13, sccb_reg_read(0x13) & ~0x02);
		sccb_reg_write(0x63, sccb_reg_read(0x63) & ~0x40);
    }
}

void camera::set_aec(bool val)
{
	aec_ = val;
	if (val)
	{
		sccb_reg_write(0x13, sccb_reg_read(0x13) | 0x01); // AEC enable
	}
	else
	{
		sccb_reg_write(0x13, sccb_reg_read(0x13) & ~0x01);
	}
}

void camera::set_framerate(int val)
{
    if (!streaming_)
        framerate_ = normalize_framerate(val);
    else
        ps3eye_debug("Can't change framerate while streaming");
}

void camera::set_test_pattern_status(bool enable)
{
    test_pattern_ = enable;
    uint8_t val = sccb_reg_read(0x0C);
    val &= ~0b00000001;
    if (test_pattern_) val |= 0b00000001; // 0x80;
    sccb_reg_write(0x0C, val);
}

void camera::set_exposure(int val)
{
    exposure_ = val;
    sccb_reg_write(0x08, exposure_ >> 7);
    sccb_reg_write(0x10, uint8_t(exposure_ << 1));
}

void camera::set_sharpness(int val)
{
    sharpness_ = val;
    sccb_reg_write(0x91, sharpness_); // vga noise / Auto De-noise threshold
    sccb_reg_write(0x8E, sharpness_); // qvga noise / De-noise threshold
}

void camera::set_contrast(int val)
{
    contrast_ = val;
    sccb_reg_write(0x9C, contrast_);
}

void camera::set_brightness(int val)
{
    brightness_ = val;
    sccb_reg_write(0x9B, brightness_);
}

void camera::set_hue(int val)
{
    hue_ = val;

	// 0x01 is AWB blue channel, not hue.
	//sccb_reg_write(0x01, (uint8_t)hue_);

	val -= 90; // exact val ranges from -90 to 90
	uint16_t huesin = (uint8_t)(sin(val) * 0x80);
	uint16_t huecos = (uint8_t)(cos(val) * 0x80);

	if (huesin < 0) {
		sccb_reg_write(0xAB, sccb_reg_read(0xAB) | 0x2); // [1:0] Hue sign bit
		huesin = -huesin;
	}
	else {
		sccb_reg_write(0xAB, sccb_reg_read(0xAB) & ~0x2);
	}
	sccb_reg_write(0xA9, (uint8_t)huecos);
	sccb_reg_write(0xAA, (uint8_t)huesin);
}

void camera::set_red_balance(int val)
{
    red_balance_ = val;
	sccb_reg_write(0x02, red_balance_); // AWB Red Channel Gain
    //sccb_reg_write(0x43, red_balance_); // BLC Red Channel Target Value
}

void camera::set_blue_balance(int val)
{
    blue_balance_ = val;
	sccb_reg_write(0x01, blue_balance_); // AWB Blue Channel Gain
    //sccb_reg_write(0x42, blue_balance_); // BLC Blue Channel Target Value
}

void camera::set_green_balance(int val)
{
    green_balance_ = val;
	sccb_reg_write(0x03, green_balance_); // AWB Green Channel Gain
    //sccb_reg_write(0x44, green_balance_); // BLC Gb Channel Target Value
}

void camera::set_flip_status(bool horizontal, bool vertical)
{
    flip_h_ = horizontal;
    flip_v_ = vertical;
    uint8_t val = sccb_reg_read(0x0c);
    val &= ~0xc0;
    if (!horizontal) val |= 0x40;
    if (!vertical) val |= 0x80;
    sccb_reg_write(0x0c, val);
}

void camera::set_gain(int val)
{
    gain_ = val;
    val = gain_;
    switch (val & 0x30)
    {
    case 0x00:
        val &= 0x0F;
        break;
    case 0x10:
        val &= 0x0F;
        val |= 0x30;
        break;
    case 0x20:
        val &= 0x0F;
        val |= 0x70;
        break;
    case 0x30:
        val &= 0x0F;
        val |= 0xF0;
        break;
    }
    sccb_reg_write(0x00, (uint8_t)val);
}

void camera::set_saturation(int val)
{
    saturation_ = val;
    sccb_reg_write(0xa7, saturation_); /* U saturation */
    sccb_reg_write(0xa8, saturation_); /* V saturation */
}

void camera::set_debug(bool value)
{
    usb_manager::instance().set_debug(value);
    _ps3eye_debug_status = value;
}

std::pair<int, int> camera::size() const
{
    switch (resolution_)
    {
    default:
    case res_VGA:
        return { 640, 480 };
    case res_QVGA:
        return { 320, 240 };
    }
}

std::vector<std::shared_ptr<camera>> list_devices()
{
    return usb_manager::instance().list_devices();
}

} // ns ps3eye
