/*
 * i2c ddc port
 */


extern int s5p_ddc_read(u8 subaddr, u8 *data, u16 len);
extern int s5p_ddc_write(u8 *data, u16 len);
