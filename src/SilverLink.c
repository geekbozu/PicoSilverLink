#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/uart.h>

#include "bsp/board.h"
#include "tusb.h"

#include "TiLink.pio.h"

int main(void) {

    gpio_set_function(1,GPIO_FUNC_UART);
    gpio_set_function(0,GPIO_FUNC_UART);
    uart_init(uart0, 115200);

    board_init();
    tusb_init();
    pio_gpio_init(pio0,2);
    pio_gpio_init(pio0,3);

    gpio_pull_up(2);
    gpio_pull_up(3);
    uint8_t tx_buf[64];
    PIO pio = pio0;

    uint offset = pio_add_program(pio, &TiLink_program);
    TiLink_program_init(pio, 0, offset, 2);
    offset = pio_add_program(pio, &pinwatch_program);
    pinwatch_program_init(pio, 1, offset, 2);
    pinwatch_program_init(pio, 2, offset, 3);

    pio_sm_set_enabled(pio, 0, true);
    pio_sm_set_enabled(pio, 1, true);
    pio_sm_set_enabled(pio, 2, true);
     while(1){
         uint count = 0;
         tud_task(); // tinyusb device task    
         while(pio_sm_get_rx_fifo_level(pio,0) & count < 32){
             tx_buf[count]=pio_sm_get(pio,0)>>24;
             count++;
         
            
         }
         if(count>0){
             
             tud_vendor_write(tx_buf,count);
             count =0;
         }     
     }
}


// Send back any data we receive from the host.
void tud_vendor_rx_cb(uint8_t itf)
{
    (void)itf;
    int t = time_us_32();
    uint count = tud_vendor_available();
    uint8_t lb_buf[64];
    const uint32_t read = tud_vendor_read(lb_buf, 32);
    int i = 0;
    while(i<count){
        if(pio_sm_get_tx_fifo_level(pio0,0)<3){
            pio_sm_put(pio0,0,lb_buf[i]);
            i++;
        }   
        if((t - time_us_32() - t) >5000){
            pio_sm_restart(pio0,0);
            return;
        }
    }
}
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
      // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP) return true;
    return false;
    //dcd_edpt_close()
}

