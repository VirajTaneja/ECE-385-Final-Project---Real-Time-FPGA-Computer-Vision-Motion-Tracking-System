#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "platform.h"
#include "xparameters.h"
#include "xil_io.h"
#include "sleep.h"
#include "xgpio.h"

#include "lw_usb/GenericMacros.h"
#include "lw_usb/GenericTypeDefs.h"
#include "lw_usb/MAX3421E.h"
#include "lw_usb/USB.h"
#include "lw_usb/usb_ch9.h"
#include "lw_usb/transfer.h"
#include "lw_usb/HID.h"

// =======================================================
// AXI GPIO MEMORY ADDRESSES
// =======================================================
#define CURSOR_X_ADDR      0x40050000
#define CURSOR_Y_ADDR      0x40050008

#define FRUIT_01_BASEADDR  0x40040000
#define FRUIT_23_BASEADDR  0x40060000
#define FRUIT_4_BASEADDR   0x40030000

// =======================================================
// GAME CONSTANTS
// =======================================================
#define SCREEN_WIDTH   640
#define SCREEN_HEIGHT  480
#define FRUIT_SIZE     64
#define GRAVITY        0.6f
#define MAX_FRUITS     5

typedef struct {
    float x;
    float y;
    float vel_x;
    float vel_y;
    int   type;
    int   is_active;
} Fruit;

Fruit fruits[MAX_FRUITS];

int cursor_x = 320;
int cursor_y = 240;

extern HID_DEVICE hid_device;

static XGpio Gpio_hex;
static BYTE addr = 1;
const char* const devclasses[] = {
    " Uninitialized", " HID Keyboard", " HID Mouse", " Mass storage"
};

void init_fruits(void) {
    for (int i = 0; i < MAX_FRUITS; i++) {
        fruits[i].is_active = 0;
    }
}

void spawn_wave(void) {
    int num_to_spawn = (rand() % 5) + 1;

    for (int i = 0; i < MAX_FRUITS; i++) {
        fruits[i].is_active = 0;
    }

    for (int i = 0; i < num_to_spawn; i++) {
        fruits[i].x = (float)((rand() % 400) + 100);
        fruits[i].y = SCREEN_HEIGHT + 10.0f;
        fruits[i].vel_x = (float)((rand() % 11) - 5) / 2.0f;
        fruits[i].vel_y = -1.0f * (float)((rand() % 8) + 16);
        fruits[i].type = rand() % 5;
        fruits[i].is_active = 1;
    }
}

void update_physics(void) {
    int active_count = 0;

    for (int i = 0; i < MAX_FRUITS; i++) {
        if (fruits[i].is_active) {
            active_count++;

            fruits[i].vel_y += GRAVITY;
            fruits[i].x += fruits[i].vel_x;
            fruits[i].y += fruits[i].vel_y;

            if (fruits[i].x < 0) {
                fruits[i].x = 0;
                fruits[i].vel_x = -fruits[i].vel_x;
            } else if (fruits[i].x > (SCREEN_WIDTH - FRUIT_SIZE)) {
                fruits[i].x = SCREEN_WIDTH - FRUIT_SIZE;
                fruits[i].vel_x = -fruits[i].vel_x;
            }

            if (fruits[i].y > SCREEN_HEIGHT + 20.0f && fruits[i].vel_y > 0) {
                fruits[i].is_active = 0;
            }
        }
    }

    if (active_count == 0) {
        spawn_wave();
    }
}

void clamp_cursor(void) {
    if (cursor_x < 0)   cursor_x = 0;
    if (cursor_x > 639) cursor_x = 639;
    if (cursor_y < 0)   cursor_y = 0;
    if (cursor_y > 479) cursor_y = 479;
}

void update_cursor_hardware(int x, int y) {
    Xil_Out32(CURSOR_X_ADDR, (uint32_t)x);
    Xil_Out32(CURSOR_Y_ADDR, (uint32_t)y);
}

uint32_t pack_fruit_data(Fruit f) {
    if (!f.is_active) {
        return (0 << 20) | (600 << 10) | 600;
    }

    uint32_t x_val    = ((uint32_t)f.x) & 0x3FF;
    uint32_t y_val    = ((uint32_t)f.y) & 0x3FF;
    uint32_t type_val = ((uint32_t)f.type) & 0x7;

    return (type_val << 20) | (y_val << 10) | x_val;
}

void update_hardware(void) {
    uint32_t packed_0 = pack_fruit_data(fruits[0]);
    uint32_t packed_1 = pack_fruit_data(fruits[1]);
    uint32_t packed_2 = pack_fruit_data(fruits[2]);
    uint32_t packed_3 = pack_fruit_data(fruits[3]);
    uint32_t packed_4 = pack_fruit_data(fruits[4]);

    Xil_Out32(FRUIT_01_BASEADDR,     packed_0);
    Xil_Out32(FRUIT_01_BASEADDR + 8, packed_1);

    Xil_Out32(FRUIT_23_BASEADDR,     packed_2);
    Xil_Out32(FRUIT_23_BASEADDR + 8, packed_3);

    Xil_Out32(FRUIT_4_BASEADDR,      packed_4);
}

BYTE GetDriverandReport(void) {
    BYTE i;
    BYTE rcode;
    BYTE device = 0xFF;
    BYTE tmpbyte;
    DEV_RECORD* tpl_ptr;

    xil_printf("Reached USB_STATE_RUNNING (0x40)\n");

    for (i = 1; i < USB_NUMDEVICES; i++) {
        tpl_ptr = GetDevtable(i);
        if (tpl_ptr->epinfo != NULL) {
            xil_printf("Device: %d", i);
            xil_printf("%s \n", devclasses[tpl_ptr->devclass]);
            device = tpl_ptr->devclass;
        }
    }

    rcode = XferGetIdle(addr, 0, hid_device.interface, 0, &tmpbyte);
    if (rcode) {
        xil_printf("GetIdle Error. Error code: %x\n", rcode);
    } else {
        xil_printf("Update rate: %x\n", tmpbyte);
    }

    xil_printf("Protocol: ");
    rcode = XferGetProto(addr, 0, hid_device.interface, &tmpbyte);
    if (rcode) {
        xil_printf("GetProto Error. Error code %x\n", rcode);
    } else {
        xil_printf("%d\n", tmpbyte);
    }

    return device;
}

void printHex(u32 data, unsigned channel) {
    XGpio_DiscreteWrite(&Gpio_hex, channel, data);
}

int main(void) {
    BYTE rcode;
    BYTE runningdebugflag = 0;
    BYTE errorflag = 0;
    BYTE device = 0xFF;

    BOOT_MOUSE_REPORT mouse_buf;
    BOOT_KBD_REPORT kbdbuf;

    init_platform();

    XGpio_Initialize(&Gpio_hex, XPAR_GPIO_USB_KEYCODE_DEVICE_ID);
    XGpio_SetDataDirection(&Gpio_hex, 1, 0x00000000);
    XGpio_SetDataDirection(&Gpio_hex, 2, 0x00000000);

    srand(12345);
    cursor_x = 320;
    cursor_y = 240;
    init_fruits();
    spawn_wave();

    xil_printf("Initializing MAX3421E...\n");
    MAX3421E_init();

    xil_printf("Initializing USB...\n");
    USB_init();

    while (1) {
        MAX3421E_Task();
        USB_Task();

        if (GetUsbTaskState() == USB_STATE_RUNNING) {
            if (!runningdebugflag) {
                runningdebugflag = 1;
                errorflag = 0;
                device = GetDriverandReport();
            }

            if (device == 1) {
                rcode = kbdPoll(&kbdbuf);
                if (rcode == 0) {
                    printHex(
                        kbdbuf.keycode[0]
                        + (kbdbuf.keycode[1] << 8)
                        + (kbdbuf.keycode[2] << 16)
                        + (kbdbuf.keycode[3] << 24),
                        1
                    );
                }
            } else if (device == 2) {
                rcode = mousePoll(&mouse_buf);
                if (rcode == 0) {
                    cursor_x += (signed char)mouse_buf.Xdispl;
                    cursor_y -= (signed char)mouse_buf.Ydispl;
                    clamp_cursor();

                    xil_printf("Mouse dx=%d dy=%d buttons=%x\n",
                        (signed char)mouse_buf.Xdispl,
                        (signed char)mouse_buf.Ydispl,
                        mouse_buf.button);
                }
            }
        } else if (GetUsbTaskState() == USB_STATE_ERROR) {
            if (!errorflag) {
                errorflag = 1;
                xil_printf("USB Error State\n");
            }
        } else {
            if (runningdebugflag) {
                runningdebugflag = 0;
                MAX3421E_init();
                USB_init();
            }
            errorflag = 0;
        }

        update_physics();
        update_hardware();
        update_cursor_hardware(cursor_x, cursor_y);

        usleep(16000);
    }

    cleanup_platform();
    return 0;
}





//#include <stdio.h>
//
//#include <stdlib.h>
//
//#include <stdint.h>
//
//#include "xparameters.h"
//
//#include "xil_io.h"
//
//#include "sleep.h"
//
//
//
//// =======================================================
//
//// AXI GPIO MEMORY ADDRESSES (From Vivado Address Editor)
//
//// =======================================================
//
//// 1. CURSOR ADDRESSES
//
//#define CURSOR_X_ADDR 0x40050000 // cursor Channel 1
//
//#define CURSOR_Y_ADDR 0x40050008 // cursor Channel 2
//
//
//
//// 2. FRUIT ADDRESSES
//
//#define FRUIT_01_BASEADDR  0x40040000 // base for fruit_data_0
//
//#define FRUIT_23_BASEADDR 0x40060000 // base for fruit_data_12
//
//#define FRUIT_4_BASEADDR 0x40030000 // base for fruit_data_34
//
//
//
//// =======================================================
//
//// GAME CONSTANTS
//
//// =======================================================
//
//#define SCREEN_WIDTH  640
//
//#define SCREEN_HEIGHT 480
//
//#define FRUIT_SIZE    64
//
//#define GRAVITY       0.6f
//
//#define MAX_FRUITS    5
//
//
//
//// =======================================================
//
//// STRUCTS & GLOBALS
//
//// =======================================================
//
//typedef struct {
//
//    float x;
//
//    float y;
//
//    float vel_x;
//
//    float vel_y;
//
//    int   type;
//
//    int   is_active;
//
//} Fruit;
//
//
//
//Fruit fruits[MAX_FRUITS];
//
//
//
//int cursor_x = 320;
//
//int cursor_y = 240;
//
//
//
//// =======================================================
//
//// INITIALIZATION & SPAWNING
//
//// =======================================================
//
//void init_fruits() {
//
//    for (int i = 0; i < MAX_FRUITS; i++) {
//
//        fruits[i].is_active = 0;
//
//    }
//
//}
//
//
//
//void spawn_wave() {
//
//    // Pick how many fruits to spawn this wave (1 to 5)
//
//    int num_to_spawn = (rand() % 5) + 1;
//
//
//
//    for (int i = 0; i < num_to_spawn; i++) {
//
//        fruits[i].x = (float)((rand() % 400) + 100);
//
//        fruits[i].y = SCREEN_HEIGHT + 10.0f;
//
//        fruits[i].vel_x = (float)((rand() % 11) - 5) / 2.0f;
//
//        fruits[i].vel_y = -1.0f * (float)((rand() % 8) + 16);
//
//        fruits[i].type = rand() % 5;
//
//        fruits[i].is_active = 1;
//
//    }
//
//}
//
//
//
//// =======================================================
//
//// PHYSICS ENGINES
//
//// =======================================================
//
//void update_physics() {
//
//    int active_count = 0;
//
//
//
//    for (int i = 0; i < MAX_FRUITS; i++) {
//
//        if (fruits[i].is_active == 1) {
//
//            active_count++;
//
//
//
//            // Apply gravity
//
//            fruits[i].vel_y += GRAVITY;
//
//            fruits[i].x += fruits[i].vel_x;
//
//            fruits[i].y += fruits[i].vel_y;
//
//
//
//            // Screen edge bounces
//
//            if (fruits[i].x < 0) {
//
//                fruits[i].x = 0;
//
//                fruits[i].vel_x = -fruits[i].vel_x;
//
//            } else if (fruits[i].x > (SCREEN_WIDTH - FRUIT_SIZE)) {
//
//                fruits[i].x = SCREEN_WIDTH - FRUIT_SIZE;
//
//                fruits[i].vel_x = -fruits[i].vel_x;
//
//            }
//
//
//
//            // Deactivate when off-screen at the bottom
//
//            if (fruits[i].y > SCREEN_HEIGHT + 20.0f && fruits[i].vel_y > 0) {
//
//                fruits[i].is_active = 0;
//
//            }
//
//        }
//
//    }
//
//
//
//    // Spawn a new wave if the screen is empty
//
//    if (active_count == 0) {
//
//        spawn_wave();
//
//    }
//
//}
//
//
//
//void clamp_cursor(void) {
//
//    if (cursor_x < 0)   cursor_x = 0;
//
//    if (cursor_x > 639) cursor_x = 639;
//
//    if (cursor_y < 0)   cursor_y = 0;
//
//    if (cursor_y > 479) cursor_y = 479;
//
//}
//
//
//
//// =======================================================
//
//// HARDWARE COMMUNICATION
//
//// =======================================================
//
//void update_cursor_hardware(int x, int y) {
//
//    Xil_Out32(CURSOR_X_ADDR, (uint32_t)x);
//
//    Xil_Out32(CURSOR_Y_ADDR, (uint32_t)y);
//
//}
//
//
//
//// Helper function to pack a fruit's data into a single 32-bit wire
//
//uint32_t pack_fruit_data(Fruit f) {
//
//    if (f.is_active == 0) {
//
//        // Send X = 600, Y = 600 to safely hide it off-screen
//
//        // 600 fits in 10 bits but avoids negative wrap-around!
//
//        return (0 << 20) | (600 << 10) | 600;
//
//    }
//
//
//
//    // Mask values to ensure they don't overlap bits
//
//    uint32_t x_val    = (uint32_t)f.x & 0x3FF;  // 10 bits
//
//    uint32_t y_val    = (uint32_t)f.y & 0x3FF;  // 10 bits
//
//    uint32_t type_val = (uint32_t)f.type & 0x7; // 3 bits
//
//
//
//    // Shift and OR together: [22:20] = Type, [19:10] = Y, [9:0] = X
//
//    return (type_val << 20) | (y_val << 10) | x_val;
//
//}
//
//
//
//void update_hardware() {
//
//    // Pack all active fruits
//
//    uint32_t packed_0 = pack_fruit_data(fruits[0]);
//
//    uint32_t packed_1 = pack_fruit_data(fruits[1]);
//
//    uint32_t packed_2 = pack_fruit_data(fruits[2]);
//
//    uint32_t packed_3 = pack_fruit_data(fruits[3]);
//
//    uint32_t packed_4 = pack_fruit_data(fruits[4]);
//
//
//
//    // Send Fruits 0 and 1 (Dual Channel)
//
//    Xil_Out32(FRUIT_01_BASEADDR, packed_0);       // Channel 1
//
//    Xil_Out32(FRUIT_01_BASEADDR + 8, packed_1);   // Channel 2
//
//
//
//    // Send Fruits 2 and 3 (Dual Channel)
//
//    Xil_Out32(FRUIT_23_BASEADDR, packed_2);       // Channel 1
//
//    Xil_Out32(FRUIT_23_BASEADDR + 8, packed_3);   // Channel 2
//
//
//
//    // Send Fruit 4 (Solo Channel)
//
//    Xil_Out32(FRUIT_4_BASEADDR, packed_4);        // Channel 1 (offset 0)
//
//}
//
//
//
//
//
//// =======================================================
//
//// MAIN LOOP
//
//// =======================================================
//
//int main() {
//
//    int dx = 2; // For your bouncing cursor test
//
//
//
//    srand(12345);
//
//    xil_printf("Starting Multi-Fruit Ninja Physics Engine...\r\n");
//
//
//
//    cursor_x = 320;
//
//    cursor_y = 240;
//
//
//
//    init_fruits();
//
//
//
//    while (1) {
//
//        // Run physics
//
//        update_physics();
//
//
//
//        // Handle cursor bouncing test logic
//
//        cursor_x += dx;
//
//        if (cursor_x < 10 || cursor_x > 630) {
//
//            dx = -dx;
//
//        }
//
//        clamp_cursor();
//
//
//
//        // Push everything to the FPGA
//
//        update_hardware();
//
//        update_cursor_hardware(cursor_x, cursor_y);
//
//
//
//        // ~60 FPS Delay
//
//        usleep(16000);
//
//    }
//
//
//
//    return 0;
//
//}
