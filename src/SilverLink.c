#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/uart.h>

#include "bsp/board.h"
#include "tusb.h"
#include "device/usbd_pvt.h"
#include "TiLink.pio.h"


bool repeating_timer_callback(struct repeating_timer *t) {
   //tud_task();
    return true;
}
int main(void) {

    gpio_set_function(1,GPIO_FUNC_UART);
    gpio_set_function(0,GPIO_FUNC_UART);
    uart_init(uart0, 115200);
 

    pio_gpio_init(pio0,2);
    pio_gpio_init(pio0,3);
    gpio_set_dir(2,false);
    gpio_set_dir(3,false);
    gpio_set_input_hysteresis_enabled(2,true);
    gpio_set_input_hysteresis_enabled(3,true);
    gpio_set_pulls(2,true,false);
    gpio_set_pulls(3,true,false);
    gpio_set_slew_rate(2,GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(3,GPIO_SLEW_RATE_SLOW);
    gpio_set_drive_strength(2,GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(3,GPIO_DRIVE_STRENGTH_2MA);
    
    board_init();
    tusb_init();
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
    struct repeating_timer timer;
    //add_repeating_timer_ms(-1, repeating_timer_callback, NULL, &timer);
   // pio_sm_put(pio0,0,0x0);

     while(1){
         uint count = 0;
         tud_task(); // tinyusb device task    
        while(pio_sm_get_rx_fifo_level(pio,0) && count < 32){
             tx_buf[count]=pio_sm_get(pio,0)>>24;
            
             uart_putc_raw(uart0,tx_buf[count]);
            
             count++;
         }
         if(count>0){
             
             tud_vendor_write(tx_buf,count);
             count =0;
         }

         
         if ( tud_vendor_available() ){
            uint8_t lb_buf[64];
            uint32_t read = tud_vendor_read(lb_buf, 32);
            absolute_time_t t = make_timeout_time_ms(2000);    
            int i = 0;
            while(i<read){
                if(pio_sm_is_tx_fifo_empty(pio0,0)){
                   
                    uart_putc_raw(uart0,lb_buf[i]);
                   
                    pio_sm_put(pio0,0,lb_buf[i]);
                    i++;
                    t = make_timeout_time_ms(2000);
                    
                }   
                tud_task();
                if(time_reached(t)){
                    pio_sm_clear_fifos(pio0,0);
                    pio_sm_restart(pio0,0);
                    pio_sm_exec(pio0,0,2);
                    break;
                }
            }
            t = make_timeout_time_ms(2000);
            while(!pio_sm_is_tx_fifo_empty(pio0,0)){
                if(time_reached(t)){
                    pio_sm_clear_fifos(pio0,0);
                    pio_sm_restart(pio0,0);
                    pio_sm_exec(pio0,0,2);
                    break;
                }
                tud_task();
            }

         }   
     }
}

// Send back any data we receive from the host.
void tud_vendor_rx_cb(uint8_t itf)
{
    (void)itf;



    
}
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
      // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP)
    {
        return true;
    }
     
    if(request->bRequest == TUSB_REQ_CLEAR_FEATURE ){
        usbd_edpt_close(rhport,request->wIndex);
    }
    //dcd_edpt_close()
}

void tud_umount_cb(){
    //usbd_edpt_close(0,0x0);
    pio_sm_clear_fifos(pio0,0);
    pio_sm_restart(pio0,0);
    pio_sm_exec(pio0,0,2);
                        
}

tud_mount_cb(){
    pio_sm_clear_fifos(pio0,0);
    pio_sm_restart(pio0,0);
    pio_sm_exec(pio0,0,2);
}
