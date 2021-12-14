#include "epd_ble_service.h"

#include <stdint.h>
#include "tl_common.h"
#include "stack/ble/ble.h"

#include "epd.h"

static uint8_t image[epd_buffer_size];
static unsigned int byte_pos = 0;

int epd_ble_handle_write(void * p) {
	rf_packet_att_write_t *req = (rf_packet_att_write_t*)p;
	uint8_t *payload = (uint8_t *) &req->value;
	unsigned int payload_len = req->l2capLen - 3;

	// switch (payload[0]) {
	// 	// Clear EPD display.
	// 	case 0x00:
	// 		memset(image, payload[1], sizeof(image));
	// 		break;
	// 	// Push buffer to display.
	// 	case 0x01:
	// 		EPD_Display(image, sizeof(image));
	// 		break;
	// 	// Set byte_pos.
	// 	case 0x02:
	// 		byte_pos = payload[1] << 8 | payload[2];
	// 		break;
	// 	// Write data to image buffer.
	// 	case 0x03:
	// 		memcpy(image + byte_pos, payload + 1, payload_len - 1);
	// 		byte_pos += payload_len - 1;
	// 		break;
	// 	// Push image buffer to display.
	// 	case 0x04:
	// 		EPD_Display(image, sizeof(image));
	// 		break;
	// 	default:
	// 		return -1;
	// }

	// char buf[64];
	// sprintf(buf, "Size: %d\n", req->l2capLen);
	// uart_write(buf);
	// uint8_t *v = &req->value;
	// memcpy(image + byte_pos, (uint8_t *) &req->value, req->l2capLen - 3);
	// byte_pos += req->l2capLen - 3;

	// if (byte_pos > 2047) {
	// 	EPD_Display(image, epd_buffer_size);
	// 	memset(image, 0xff, epd_buffer_size);
	// 	byte_pos = 0;
	// }
	// for (int i = 0; i < 20; i++) {
	// 	// sprintf(buf, "0x%02x ", req->dat[i]);
	// 	sprintf(buf, "0x%02x ", *(v + i));
	// 	uart_write(buf);
	// }
	// uart_write("\n");
	// return 0;
	return 0;
}
