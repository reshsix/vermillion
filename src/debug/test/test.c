/*
This file is part of vermillion.

Vermillion is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Vermillion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <debug/assert.h>

void test_general_types(void);
//void test_general_macros(void);
void test_general_mem(void);
void test_general_str(void);
void test_general_dict(void);

void test_environ_fork(void);
void test_environ_state(void);
void test_environ_generator(void);

void test_thread_mutex(void);
void test_thread_thread(void);
void test_thread_channel(void);
void test_thread_critical(void);
void test_thread_implicit(void);
void test_thread_semaphore(void);

//void test_hal_base_dev(void);
//void test_hal_base_drv(void);

//void test_hal_generic_block(void);
//void test_hal_generic_stream(void);

//void test_hal_classes_fs(void);
//void test_hal_classes_pic(void);
//void test_hal_classes_spi(void);
//void test_hal_classes_gpio(void);
//void test_hal_classes_uart(void);
//void test_hal_classes_video(void);
//void test_hal_classes_audio(void);
//void test_hal_classes_timer(void);
//void test_hal_classes_storage(void);

void test_system_log(void);
//void test_system_wheel(void);

//void test_debug_exit(void);
//void test_debug_assert(void);
//void test_debug_profile(void);

extern void
test_all()
{
    test_general_types();
    //test_general_macros();
    test_general_mem();
    test_general_str();
    test_general_dict();

    test_environ_fork();
    test_environ_state();
    test_environ_generator();

    test_thread_mutex();
    test_thread_thread();
    test_thread_channel();
    test_thread_critical();
    test_thread_implicit();
    test_thread_semaphore();

    //test_hal_base_dev();
    //test_hal_base_drv();

    //test_hal_generic_block();
    //test_hal_generic_stream();

    //test_hal_classes_fs();
    //test_hal_classes_pic();
    //test_hal_classes_spi();
    //test_hal_classes_gpio();
    //test_hal_classes_uart();
    //test_hal_classes_video();
    //test_hal_classes_audio();
    //test_hal_classes_timer();
    //test_hal_classes_storage();

    test_system_log();
    //test_system_wheel();

    //test_debug_exit();
    //test_debug_assert();
    //test_debug_profile();
}
