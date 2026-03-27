/**
 * ****************************************************************************************
 *
 * @file user_custs1_impl.c
 *
 * @brief Mã nguồn thực thi Server Custom1 cho dự án ngoại vi.
 *
 * Bản quyền (C) 2015-2019 Dialog Semiconductor.
 * Chương trình máy tính này bao gồm Thông tin Bảo mật, Độc quyền
 * của Dialog Semiconductor. Bảo lưu mọi quyền.
 *
 * ****************************************************************************************
 */

/*
 * CÁC FILE INCLUDE
 * ****************************************************************************************
 */

#include "gpio.h"
#include "app_api.h"
#include "app.h"
#include "prf_utils.h"
#include "uart_utils.h"
#include "custs1.h"
#include "custs1_task.h"
#include "user_custs1_def.h"
#include "user_custs1_impl.h"
#include "user_periph_setup.h"
#include "user_callback_config.h"
#include "EPD_2in13_V2.h"
#include "etime.h"
#include "crc32.h"
#include "app_bass.h"
#include "battery.h"
#include "user_ota.h"
#include "fonts.h"
#include "GUI_Paint.h"
#include "lunar.h"
#include "display_manager.h"
#include "time_display.h"
#include "calendar_display.h"
#include "analog_clock.h"
#include "custom_drawing.h"
#include "fabric_record_display.h"
#include "co_bt.h"
#include <stdio.h>
#include <time.h>
#include "ImageData.h"
#include "spi_flash.h"
#include "time_display.h"

/*
 * ĐỊNH NGHĨA BIẾN TOÀN CỤC
 * ****************************************************************************************
 */
#define APP_PERIPHERAL_CTRL_TIMER_DELAY 10
#define APP_PERIPHERAL_CTRL_TIMER_DELAY_MINUTES 6000
#define Time_To_Refresh 0

#define IMG_HEADER_SIGNATURE1 0x70
#define IMG_HEADER_SIGNATURE2 0x55
#define IMG_HEADER_VALID 0x55
#define IMG_HEADER_ADDRESS 0x20000

// Biến toàn cục để theo dõi cập nhật (Retained memory)
static uint32_t last_update_time __SECTION_ZERO("retention_mem_area0");
static uint8_t last_minute __SECTION_ZERO("retention_mem_area0");
uint8_t current_display_mode __SECTION_ZERO("retention_mem_area0"); 

/// Header của hình ảnh
typedef struct
{
    uint8_t signature[2];
    uint8_t validflag;
    uint8_t imageid;
    uint32_t code_size;
    uint32_t CRC;
} img_header_t;

img_header_t imgheader __SECTION_ZERO("retention_mem_area0");
ke_msg_id_t timer_used __SECTION_ZERO("retention_mem_area0");      //@RETENTION MEMORY
uint16_t indication_counter __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
uint16_t non_db_val_counter __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY

#define epd_buffer_size (EPD_2IN13_V2_WIDTH * EPD_2IN13_V2_HEIGHT / 8)
#define buffer_size (EPD_2IN13_V2_WIDTH * EPD_2IN13_V2_HEIGHT / 4)
uint32_t byte_pos __SECTION_ZERO("retention_mem_area0");
uint8_t epd_buffer[buffer_size] __SECTION_ZERO("retention_mem_area0");
uint16_t index __SECTION_ZERO("retention_mem_area0");
uint8_t step __SECTION_ZERO("retention_mem_area0");
ke_msg_id_t timer_used_min __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
uint32_t current_unix_time __SECTION_ZERO("retention_mem_area0");
uint8_t time_offset __SECTION_ZERO("retention_mem_area0");
uint16_t time_refresh_count __SECTION_ZERO("retention_mem_area0");
uint8_t isconnected __SECTION_ZERO("retention_mem_area0");
tm_t g_tm __SECTION_ZERO("retention_mem_area0");
uint8_t is_part __SECTION_ZERO("retention_mem_area0");

void bls_att_pushNotifyData(uint16_t attHandle, uint8_t *p, uint8_t len);
void display(void);

#define IMAGE_WIDTH  70
#define IMAGE_HEIGHT 70
#define IMAGE_BUFFER_SIZE ((IMAGE_WIDTH * IMAGE_HEIGHT + 7) / 8)
UBYTE Image_Buffer[IMAGE_BUFFER_SIZE];

void DisplayCustomImage(void)
{
    // Hàm này hiện không được dùng trong display loop
    // Ảnh được upload trực tiếp vào epd_buffer qua BLE (lệnh 0x02/0x03/0x01)
    // display() pipeline sẽ hiển thị nội dung epd_buffer đó
}

// Display functions moved to separate components

/**
 * @brief Hàm cập nhật hiển thị sau khi sửa đổi, hỗ trợ đồng hồ kim
 */
void do_display_update_with_analog_clock(void)
{
    char buf[50];
    char buf2[50];
    
    transformTime(current_unix_time, &g_tm);
    
    transformTime(current_unix_time, &g_tm);
    
    bool force_redraw = (last_update_time == 0 || (current_unix_time - last_update_time) > 3600 || g_tm.tm_min == 0);
    bool minute_changed = (last_minute != g_tm.tm_min);
    
    if (force_redraw) last_update_time = current_unix_time;
    if (minute_changed) last_minute = g_tm.tm_min;
    
    // Dummy data for fabric record
    static fabric_record_t current_fabric_record = {
        .width = "62/66 NEW",
        .staff = "Tai",
        .po = "302J07",
        .relax_date = "12/3 9h",
        .ok_date = "14/3",
        .item = "Tie: 0464",
        .color = "Caviar",
        .lot = "4/296 6/985",
        .buy = "1224",
        .roll = "101",
        .yds = "489",
        .note = "T4"
    };
    
    switch (current_display_mode)
    {
        
        case DISPLAY_MODE_TIME: // Mode đồng hồ mặc định
            draw_time_page(current_unix_time, force_redraw || minute_changed);
            break;
        
        case DISPLAY_MODE_IMAGE:
            // KHÔNG vẽ gì vào epd_buffer!
            // epd_buffer đã chứa ảnh được upload qua BLE (lệnh 0x02/0x03)
            // display() pipeline sẽ hiển thị đúng nội dung đó
            // Skip luôn phần vẽ pin phía dưới bằng cách return sớm
            arch_printf("IMAGE mode: display raw epd_buffer\n");
            return;
                
        case DISPLAY_MODE_CALENDAR:
            draw_calendar_page(current_unix_time, force_redraw || minute_changed);
            break;
            
        case DISPLAY_MODE_CALENDAR_ANALOG:
            draw_calendar_with_analog_clock(current_unix_time, force_redraw || minute_changed);
            break;

        case DISPLAY_MODE_FABRIC_RECORD:
            draw_fabric_record_page(&current_fabric_record, force_redraw || minute_changed);
            break;

        default:
            current_display_mode = DISPLAY_MODE_TIME;
            draw_time_page(current_unix_time, force_redraw || minute_changed);
            break;
    }
    
		// Vê giao dien pin dung chung
		// 1. Khung pin (giữ nguyên)
        Paint_DrawRectangle(196, 4, 210, 11, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

        // 2. Đầu cực chuyển sang trái
        Paint_DrawRectangle(194, 6, 195, 10, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // 3. Ruột pin (fill từ phải -> trái)
        if (cur_batt_level > 0)
        {
            int width = (cur_batt_level * 10 / 100); // chiều dài fill

            Paint_DrawRectangle(208 - width, 6, 208, 10,
                                BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    
    // // 3. Hiển thị số bằng EPD_DrawUTF8 vào TRONG khung
    // sprintf(buf, "%d", cur_batt_level);
    // // Nếu pin đầy (>60%), dùng chữ Trắng trên nền Đen của ruột pin
    // if (cur_batt_level > 60) {
    //     EPD_DrawUTF8(189, 3, 0, buf, EPD_ASCII_7X12, 0, WHITE, BLACK);
    // } else {
    //     EPD_DrawUTF8(189, 3, 0, buf, EPD_ASCII_7X12, 0, BLACK, WHITE);
    // }
    
    // Hiển thị kết nối BLE (Sửa Y từ 150 về 1)
    // if (isconnected == 1)
    // {
    //     EPD_DrawUTF8(200, 1, 0, "B", EPD_ASCII_7X12, 0, BLACK, WHITE);
    // }

    if (g_tm.tm_min == 0)
    {
        is_part = 0;
    }
    step = 0;
}


/*
 * ĐỊNH NGHĨA HÀM (FUNCTION DEFINITIONS)
 * ****************************************************************************************
 */
void spi_flash_peripheral_init(void)
{
    spi_flash_release_from_power_down();
    uint8_t dev_id;
    spi_flash_auto_detect(&dev_id);
}

void do_img_save(void)
{
    uint32_t actual_size;
    if (imgheader.signature[0] == IMG_HEADER_SIGNATURE1 && imgheader.signature[1] == IMG_HEADER_SIGNATURE2 && imgheader.validflag != IMG_HEADER_VALID)
    {
        imgheader.validflag = IMG_HEADER_VALID;
        img_header_t *imgheadertmp;
        spi_flash_peripheral_init();

        uint8_t buf[20] = {0};
        int8_t status = spi_flash_read_data(buf, IMG_HEADER_ADDRESS, sizeof(img_header_t), &actual_size);
        arch_printf("status0:%d\n", status);
        imgheadertmp = (img_header_t *)buf;
        // Đưa SPI flash vào chế độ cực thấp điện năng (ultra deep power down)
        spi_flash_ultra_deep_power_down();
    }
}

/**
 * @brief Hàm công việc mỗi phút sau khi sửa đổi
 */
void do_min_work_with_analog_clock(void)
{
    timer_used_min = app_easy_timer(APP_PERIPHERAL_CTRL_TIMER_DELAY_MINUTES, do_min_work_with_analog_clock);
    current_unix_time += time_offset;
    time_offset = 60;
    
    arch_printf("current_unix_time:%d\n", current_unix_time);
    
    // Sử dụng hàm cập nhật hiển thị mới
    do_display_update_with_analog_clock();
    
    if (step == 0)
    {
        step = 1;
        display();
    }
}


void my_app_on_db_init_complete(void)
{
    crc32_init(DEFAULT_POLY);
    arch_printf("crc32_init\n");
    CALLBACK_ARGS_0(user_default_app_operations.default_operation_adv)
    arch_printf("my_app_on_db_init_complete_modified\n");
    
    // Khởi tạo chế độ hiển thị mặc định (Tạm thời test lịch)
    current_display_mode = DISPLAY_MODE_CALENDAR;
    last_update_time = 0;
    last_minute = 255;
    
    timer_used_min = app_easy_timer(100, do_min_work_with_analog_clock);
    spi_flash_peripheral_init();
    uint32_t actual_size;
    int8_t status = spi_flash_read_data((uint8_t *)&imgheader, IMG_HEADER_ADDRESS, sizeof(img_header_t), &actual_size);
    arch_printf("imgheader read:%d,status:%d", actual_size, status);
    if (imgheader.signature[0] == IMG_HEADER_SIGNATURE1 && imgheader.signature[1] == IMG_HEADER_SIGNATURE2 && imgheader.validflag == IMG_HEADER_VALID)
    {
        spi_flash_read_data(epd_buffer, IMG_HEADER_ADDRESS + sizeof(img_header_t), sizeof(epd_buffer), &actual_size);
        arch_printf("epd_buffer read:%d\n", actual_size);
    }
    spi_flash_ultra_deep_power_down();
    cur_batt_level = 101;
    app_batt_lvl();
}


void user_svc1_ctrl_wr_ind_handler(ke_msg_id_t const msgid,
                                   struct custs1_val_write_ind const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    uint8_t val = 0;
    memcpy(&val, &param->value[0], param->length);

    if (val != CUSTS1_CP_ADC_VAL1_DISABLE)
    {
        timer_used = app_easy_timer(APP_PERIPHERAL_CTRL_TIMER_DELAY, app_adcval1_timer_cb_handler);
    }
    else
    {
        if (timer_used != EASY_TIMER_INVALID_TIMER)
        {
            app_easy_timer_cancel(timer_used);
            timer_used = EASY_TIMER_INVALID_TIMER;
        }
    }
}

void display(void)
{
    time_refresh_count = 0;
    uint8_t isbusy = 1;
    uint32_t delay = APP_PERIPHERAL_CTRL_TIMER_DELAY;
    uint8_t out_buffer[2];
    spi_set_bitmode(SPI_MODE_8BIT);
    spi_cs_high();
    switch (step)
    {
    case 1:
        cur_batt_level = 100;
        app_batt_lvl();
        arch_printf("epd_init\n");
        EPD_GPIO_init();
        EPD_2IN13_V2_Init(is_part);
        step++;
        break;
    case 2:
        if (is_part == 0)
        {
            EPD_2IN13_V2_DisplayPartBaseImage(epd_buffer);
        }
        else
        {
            EPD_2IN13_V2_DisplayPart(epd_buffer);
        }

        step++;
        break;
    case 3:
        if (is_part == 0)
        {
            is_part = 1;
            EPD_2IN13_V2_TurnOnDisplay();
        }
        else
        {
            EPD_2IN13_V2_TurnOnDisplayPart();
        }

        step++;
        break;
    case 4:
        app_batt_lvl();
        if (EPD_BUSY == 0)
        {
            isbusy = 0;
            step = 0;
            arch_printf("epd_sleep\n");
            EPD_2IN13_V2_Sleep();
            out_buffer[0] = 0xff;
            out_buffer[1] = 0xff;
            bls_att_pushNotifyData(SVC1_IDX_LED_STATE_VAL, out_buffer, 2);
        }

        break;
    default:
        break;
    }

    if (isbusy && (step != 0))
    {
        app_easy_timer(delay, display);
    }
    else
    {
        app_easy_timer_cancel(timer_used);
    }
}

void bls_att_pushNotifyData(uint16_t attHandle, uint8_t *p, uint8_t len)
{
    struct custs1_val_ntf_ind_req *req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_NTF_REQ,
                                                          prf_get_task_from_id(TASK_ID_CUSTS1),
                                                          TASK_APP,
                                                          custs1_val_ntf_ind_req,
                                                          DEF_SVC1_ADC_VAL_1_CHAR_LEN);

    req->handle = attHandle;
    req->length = len;
    req->notification = true;
    memcpy(req->value, p, len);

    ke_msg_send(req);
}

void user_svc1_led_wr_ind_handler(ke_msg_id_t const msgid,
                                  struct custs1_val_write_ind const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)

{
    const uint8_t *payload = param->value;
    uint16_t payload_len = param->length;
    uint8_t out_buffer[20] = {0};

    // Lệnh 0x02 và 0x03: set IMAGE mode và ghi data người dùng - bypass guard step
    if (param->value[0] == 0x02)
    {
        ASSERT_MIN_LEN(payload_len, 3);
        byte_pos = payload[1] << 8 | payload[2];
        // Set IMAGE mode ngay khi bắt đầu upload để minute timer không overwrite epd_buffer
        current_display_mode = DISPLAY_MODE_IMAGE;
        arch_printf("Upload start: IMAGE protected, byte_pos=%d\n", byte_pos);
        return;
    }
    if (param->value[0] == 0x03)
    {
        if ((payload[2] << 8 | payload[3]) + payload_len - 4 >= epd_buffer_size + 1)
        {
            out_buffer[0] = 0x00;
            out_buffer[1] = 0x00;
            bls_att_pushNotifyData(SVC1_IDX_LED_STATE_VAL, out_buffer, 2);
            return;
        }
        if (payload[1] == 0xff)
        {
            memcpy(epd_buffer + (payload[2] << 8 | payload[3]), payload + 4, payload_len - 4);
        }
        out_buffer[0] = payload_len >> 8;
        out_buffer[1] = payload_len & 0xff;
        bls_att_pushNotifyData(SVC1_IDX_LED_STATE_VAL, out_buffer, 2);
        return;
    }
    // Lệnh 0x01: trigger display - bypass guard, hủy pipeline cũ nếu đang chạy
    if (param->value[0] == 0x01)
    {
        arch_printf("start EPD_Display, step=%d\r\n", step);
        if (step != 0)
        {
            app_easy_timer_cancel(timer_used);
            step = 0;
            arch_printf("Cancelled old pipeline, forcing IMAGE display\r\n");
        }
        imgheader.signature[0] = IMG_HEADER_SIGNATURE1;
        imgheader.signature[1] = IMG_HEADER_SIGNATURE2;
        imgheader.CRC = 0xFFFFFFFF;
        imgheader.CRC = crc32(imgheader.CRC, epd_buffer, sizeof(epd_buffer));
        imgheader.CRC ^= 0xFFFFFFFF;
        imgheader.code_size = sizeof(epd_buffer);
        imgheader.validflag = 0;
        current_display_mode = DISPLAY_MODE_IMAGE;
        last_update_time = current_unix_time;
        is_part = 0;
        step = 1;
        app_easy_timer(APP_PERIPHERAL_CTRL_TIMER_DELAY, display);
        return;
    }

    // Các lệnh khác cần EPD rảnh (step == 0)
    if (step != 0)
    {
        out_buffer[0] = 0x00;
        out_buffer[1] = 0x00;
        bls_att_pushNotifyData(SVC1_IDX_LED_STATE_VAL, out_buffer, 2);
        return;
    }
    switch (param->value[0])
    {
    // Xóa màn hình EPD.
    case 0x00:
        ASSERT_MIN_LEN(payload_len, 2);
        return;
    // 0x01, 0x02, 0x03 đã được xử lý bên trên (bypass guard step!=0)
    case 0x04: // Giải mã và hiển thị hình ảnh TIFF
        // param_update_request(1);
        return;
    case 0xAA:
        do_img_save();
        break;
    case 0xAB:
        platform_reset(RESET_NO_ERROR);
        break;
    case 0xE3: // Chuyển sang chế độ hiển thị thời gian
        current_display_mode = DISPLAY_MODE_TIME;
        last_update_time = 0; // Buộc vẽ lại
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        break;
    case 0xE4: // Chuyển sang chế độ hiển thị lịch (đồng hồ số)
        current_display_mode = DISPLAY_MODE_CALENDAR;
        last_update_time = 0; // Buộc vẽ lại
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        break;
    case 0xE5: // Chuyển sang chế độ hiển thị lịch (đồng hồ kim)
        current_display_mode = DISPLAY_MODE_CALENDAR_ANALOG;
        last_update_time = 0; // Buộc vẽ lại
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        break;
    case 0xE6: // Chuyển sang chế độ hiển thị thẻ kho / ghi chép xả vải
        current_display_mode = DISPLAY_MODE_FABRIC_RECORD;
        last_update_time = 0; // Buộc vẽ lại
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        break;
    case 0xE1: // Lệnh chuyển đổi chế độ kèm chỉ số (từ app)
        if (payload_len >= 2)
        {
            uint8_t mode_idx = payload[1];
            // Mapping app-level: đúng với những gì app gửi
            // 0x00=IMAGE, 0x01=CALENDAR, 0x02=TIME
            // 0x03=CALENDAR_ANALOG, 0x04=CLOCK, 0x05=FABRIC_RECORD
            if (mode_idx == 0x00) current_display_mode = DISPLAY_MODE_IMAGE;
            else if (mode_idx == 0x01) current_display_mode = DISPLAY_MODE_CALENDAR;
            else if (mode_idx == 0x02) current_display_mode = DISPLAY_MODE_TIME;
            else if (mode_idx == 0x03) current_display_mode = DISPLAY_MODE_CALENDAR_ANALOG;
            else if (mode_idx == 0x05) current_display_mode = DISPLAY_MODE_FABRIC_RECORD;
            
            last_update_time = 0;
            do_display_update_with_analog_clock();
            is_part = 0;
            step = 1;
            display();
            arch_printf("E1: mode_idx=%d, display_mode=%d\n", mode_idx, current_display_mode);
        }
        break;
    default:
        return;
    }
}

/**
 * @brief Hàm xử lý lệnh ghi cổng nối tiếp (Custom Service 2) đã sửa đổi, thêm chuyển đổi chế độ đồng hồ kim
 */
void user_svc2_wr_ind_handler(ke_msg_id_t const msgid,
                              struct custs1_val_write_ind const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    arch_printf("cmd HEX %d:", param->length);
    for (int i = 0; i < param->length; i++)
    {
        arch_printf("%02X", param->value[i]);
    }
    arch_printf("\r\n");
    
    if ((param->value[0] == 0xDD) && (param->length >= 5))
    {
        // Logic thiết lập thời gian giữ nguyên
        current_unix_time = (param->value[1] << 24) + (param->value[2] << 16) + (param->value[3] << 8) + (param->value[4] & 0xff);
        tm_t tm = {0};
        transformTime(current_unix_time, &tm);
        app_easy_timer_cancel(timer_used_min);
        time_offset = 60 - tm.tm_sec;
        timer_used_min = app_easy_timer(time_offset * 100, do_min_work_with_analog_clock);
        
        // Đặt lại thời gian cập nhật, buộc vẽ lại
        last_update_time = 0;
        
        arch_printf("%d-%02d-%02d %02d:%02d:%02d %d\n", tm.tm_year + YEAR0,
                    tm.tm_mon + 1,
                    tm.tm_mday,
                    tm.tm_hour,
                    tm.tm_min,
                    tm.tm_sec,
                    tm.tm_wday);
    }
    else if (param->value[0] == 0xAA)
    {
        platform_reset(RESET_NO_ERROR);
    }
    else if (param->value[0] == 0xAB)
    {
        // do_img_save();
    }
    else if (param->value[0] == 0xE2)
    {
        // Làm mới chế độ hiển thị hiện tại
        last_update_time = 0; // Buộc vẽ lại
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        arch_printf("E2: Refresh display, current_display_mode=%d\n", current_display_mode);
    }
    else if (param->value[0] == 0xE3)
    {
        // Chuyển sang chế độ hiển thị thời gian
        current_display_mode = DISPLAY_MODE_TIME;
        last_update_time = 0; // Buộc vẽ lại
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        arch_printf("Switched to TIME display mode\n");
    }
    else if (param->value[0] == 0xE4)
    {
        // Chuyển sang chế độ hiển thị lịch (đồng hồ số)
        current_display_mode = DISPLAY_MODE_CALENDAR;
        last_update_time = 0; // Buộc vẽ lại
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        arch_printf("Switched to CALENDAR display mode\n");
    }
    else if (param->value[0] == 0xE5)
    {
        // Thêm mới: Chuyển sang chế độ hiển thị lịch (đồng hồ kim)
        current_display_mode = DISPLAY_MODE_CALENDAR_ANALOG;
        last_update_time = 0; // Buộc vẽ lại
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        arch_printf("Switched to CALENDAR ANALOG display mode\n");
    }
    else if (param->value[0] == 0xE6) 
    {
        current_display_mode = DISPLAY_MODE_FABRIC_RECORD;
        last_update_time = 0;
        do_display_update_with_analog_clock();
        is_part = 0;
        step = 1;
        display();
        arch_printf("Switched to FABRIC RECORD mode\n");
    }
    else if (param->value[0] == 0xE1)
    {
        // Chuyển sang chế độ hiển thị kèm chỉ số (từ app)
        if (param->length >= 2)
        {
            uint8_t mode_idx = param->value[1];
            // Mapping app-level: đúng với những gì app gửi
            // 0x00=IMAGE, 0x01=CALENDAR, 0x02=TIME
            // 0x03=CALENDAR_ANALOG, 0x04=CLOCK, 0x05=FABRIC_RECORD
            if (mode_idx == 0x00) current_display_mode = DISPLAY_MODE_IMAGE;
            else if (mode_idx == 0x01) current_display_mode = DISPLAY_MODE_CALENDAR;
            else if (mode_idx == 0x02) current_display_mode = DISPLAY_MODE_TIME;
            else if (mode_idx == 0x03) current_display_mode = DISPLAY_MODE_CALENDAR_ANALOG;
            else if (mode_idx == 0x04) current_display_mode = DISPLAY_MODE_FABRIC_RECORD;
            
            last_update_time = 0;
            do_display_update_with_analog_clock();
            is_part = 0;
            step = 1;
            display();
            arch_printf("E1: mode_idx=%d, display_mode=%d\n", mode_idx, current_display_mode);
        }
    }
}


void user_svc1_long_val_cfg_ind_handler(
    ke_msg_id_t const msgid, struct custs1_val_write_ind const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  
}

void user_svc1_long_val_wr_ind_handler(ke_msg_id_t const msgid,
                                       struct custs1_val_write_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
}

void user_svc1_long_val_ntf_cfm_handler(ke_msg_id_t const msgid,
                                        struct custs1_val_write_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
}

void user_svc1_adc_val_1_cfg_ind_handler(ke_msg_id_t const msgid,
                                         struct custs1_val_write_ind const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
}

void user_svc1_adc_val_1_ntf_cfm_handler(ke_msg_id_t const msgid,
                                         struct custs1_val_write_ind const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
}

void user_svc1_button_cfg_ind_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_write_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
}

void user_svc1_button_ntf_cfm_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_write_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
}

void user_svc1_indicateable_cfg_ind_handler(ke_msg_id_t const msgid,
                                            struct custs1_val_write_ind const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
}

void user_svc1_indicateable_ind_cfm_handler(ke_msg_id_t const msgid,
                                            struct custs1_val_write_ind const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
}

void user_svc1_long_val_att_info_req_handler(ke_msg_id_t const msgid,
                                             struct custs1_att_info_req const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    struct custs1_att_info_rsp *rsp = KE_MSG_ALLOC(CUSTS1_ATT_INFO_RSP,
                                                   src_id,
                                                   dest_id,
                                                   custs1_att_info_rsp);
    // Provide the connection index.
    rsp->conidx = app_env[param->conidx].conidx;
    // Provide the attribute index.
    rsp->att_idx = param->att_idx;
    // Provide the current length of the attribute.
    rsp->length = 0;
    // Provide the ATT error code.
    rsp->status = ATT_ERR_NO_ERROR;

    ke_msg_send(rsp);
}

void user_svc1_rest_att_info_req_handler(ke_msg_id_t const msgid,
                                         struct custs1_att_info_req const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    struct custs1_att_info_rsp *rsp = KE_MSG_ALLOC(CUSTS1_ATT_INFO_RSP,
                                                   src_id,
                                                   dest_id,
                                                   custs1_att_info_rsp);
    // Provide the connection index.
    rsp->conidx = app_env[param->conidx].conidx;
    // Provide the attribute index.
    rsp->att_idx = param->att_idx;
    // Force current length to zero.
    rsp->length = 0;
    // Provide the ATT error code.
    rsp->status = ATT_ERR_WRITE_NOT_PERMITTED;

    ke_msg_send(rsp);
}

void app_adcval1_timer_cb_handler()
{
    // struct custs1_val_ntf_ind_req *req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_NTF_REQ,
    //                                                       prf_get_task_from_id(TASK_ID_CUSTS1),
    //                                                       TASK_APP,
    //                                                       custs1_val_ntf_ind_req,
    //                                                       DEF_SVC1_ADC_VAL_1_CHAR_LEN);

    // // ADC value to be sampled
    // static uint16_t sample __SECTION_ZERO("retention_mem_area0");
    // sample = (sample <= 0xffff) ? (sample + 1) : 0;

    // // req->conhdl = app_env->conhdl;
    // req->handle = SVC1_IDX_ADC_VAL_1_VAL;
    // req->length = DEF_SVC1_ADC_VAL_1_CHAR_LEN;
    // req->notification = true;
    // memcpy(req->value, &sample, DEF_SVC1_ADC_VAL_1_CHAR_LEN);

    // ke_msg_send(req);

    if (ke_state_get(TASK_APP) == APP_CONNECTED)
    {
        // Set it once again until Stop command is received in Control Characteristic
        timer_used = app_easy_timer(APP_PERIPHERAL_CTRL_TIMER_DELAY, app_adcval1_timer_cb_handler);
    }
}

void user_svc3_read_non_db_val_handler(ke_msg_id_t const msgid,
                                       struct custs1_value_req_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Increase value by one
    non_db_val_counter++;

    // struct custs1_value_req_rsp *rsp = KE_MSG_ALLOC_DYN(CUSTS1_VALUE_REQ_RSP,
    //                                                     prf_get_task_from_id(TASK_ID_CUSTS1),
    //                                                     TASK_APP,
    //                                                     custs1_value_req_rsp,
    //                                                     DEF_SVC3_READ_VAL_4_CHAR_LEN);

    // // Provide the connection index.
    // rsp->conidx = app_env[param->conidx].conidx;
    // // Provide the attribute index.
    // rsp->att_idx = param->att_idx;
    // // Force current length to zero.
    // rsp->length = sizeof(non_db_val_counter);
    // // Provide the ATT error code.
    // rsp->status = ATT_ERR_NO_ERROR;
    // // Copy value
    // memcpy(&rsp->value, &non_db_val_counter, rsp->length);
    // // Send message
    // ke_msg_send(rsp);
}
