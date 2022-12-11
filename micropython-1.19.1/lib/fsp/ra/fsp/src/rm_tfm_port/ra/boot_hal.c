/*
 * Copyright (c) 2019-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "target_cfg.h"
#include "cmsis.h"
#include "boot_hal.h"
#include "Driver_Flash.h"
#include "region_defs.h"
#include "bootutil/bootutil_log.h"
#include "crypto_keys.h"

#include "r_flash_hp.h"
#include "r_flash_api.h"

#include "psa/crypto.h"
#if defined(MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT)
 #include "platform.h"
#endif

#if defined(__ARMCC_VERSION)
__attribute__((naked)) void boot_clear_bl2_ram_area (void)
{
    __asm volatile (
        ".syntax unified                             \n"
        "mov     r0, #0                              \n"
        "ldr     r1, =Image$$DATA$$Base           \n"
        "ldr     r2, =Image$$ARM_LIB_HEAP$$ZI$$Limit \n"
        "subs    r2, r2, r1                          \n"
        "Loop:                                       \n"
        "subs    r2, #4                              \n"
        "itt     ge                                  \n"
        "strge   r0, [r1, r2]                        \n"
        "bge     Loop                                \n"
        "bx      lr                                  \n"
        : : : "r0", "r1", "r2", "memory"
        );
}

#elif defined(__GNUC__)
__attribute__((naked)) void boot_clear_bl2_ram_area (void)
{
    __asm volatile (
        ".syntax unified                             \n"
        "mov     r0, #0                              \n"
        "ldr     r1, =__data_start__                 \n"
        "ldr     r2, =__HeapLimit                    \n"
        "subs    r2, r2, r1                          \n"
        "Loop:                                       \n"
        "subs    r2, #4                              \n"
        "itt     ge                                  \n"
        "strge   r0, [r1, r2]                        \n"
        "bge     Loop                                \n"
        "bx      lr                                  \n"
        : : : "r0", "r1", "r2", "memory"
        );
}

#elif defined(__ICCARM__)
extern uint32_t HEAP$$Limit;
extern uint32_t data$$Base;
__attribute__((naked)) void boot_clear_bl2_ram_area (void)
{
    __asm volatile (
        "mov     r0, #0                              \n"
        "ldr     r1, = data$$Base                    \n"
        "ldr     r2, = HEAP$$Limit                   \n"
        "subs    r2, r2, r1                          \n"
        "Loop:                                       \n"
        "subs    r2, #4                              \n"
        "itt     ge                                  \n"
        "strge   r0, [r1, r2]                        \n"
        "bge     Loop                                \n"
        "bx      lr                                  \n"
        : : : "r0", "r1", "r2", "memory"
        );
}

#endif

/*
 * The below structure contains the hard coded version of the ECDSA P-256 secret key in:
 * platform/ext/common/template/tfm_initial_attestation_key.pem
 *
 * As a P-256 key, the private key is 32 bytes long.
 *
 * This key is used to sign the initial attestation token.
 * The secret key is stored in raw format, without any encoding(ASN.1, COSE).
 *
 * #######  DO NOT USE THIS KEY IN PRODUCTION #######
 */

const iak_data_t iak_data =
{
    {
        0xA9, 0xB4, 0x54, 0xB2, 0x6D, 0x6F, 0x90, 0xA4,
        0xEA, 0x31, 0x19, 0x35, 0x64, 0xCB, 0xA9, 0x1F,
        0xEC, 0x6F, 0x9A, 0x00, 0x2A, 0x7D, 0xC0, 0x50,
        0x4B, 0x92, 0xA1, 0x93, 0x71, 0x34, 0x58, 0x5F
    },
    MAX_IAK_BYTES,
    PSA_ECC_CURVE_SECP_R1
};

/*
 * The below structure contains the hard coded HUK
 * and its size.
 *
 * #######  DO NOT USE THIS KEY IN PRODUCTION #######
 */

const huk_data_t huk_data =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    MAX_HUK_BYTES
};

/*
 * The section 'fsp_dtc_vector_table' is reused to store boot data.
 * This is done to avoid creating a BL2 specific linker script.
 */
uint8_t g_dummy_reserve_for_tfm[BOOT_TFM_SHARED_DATA_TOTAL_SIZE] BSP_PLACE_IN_SECTION(".fsp_dtc_vector_table") = {0};

#define FAW_START_ADDR    (0xFFFC)
#define FAW_END_ADDR      (0x200000)

/* Flash device name must be specified by target */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME;

/* FSP structures required by uart and flash drivers */
extern flash_hp_instance_ctrl_t g_tfm_fsp_flash_ctrl;

static void flash_FAW_Set (uint32_t start_addr, uint32_t end_addr)
{
    int ret_val = 0;

    volatile uint32_t faws = (uint32_t) ((R_FACI_HP->FAWMON_b.FAWS) << 13);
    volatile uint32_t fawe = (uint32_t) ((R_FACI_HP->FAWMON_b.FAWE) << 13);

    if (faws == fawe)
    {
        BOOT_LOG_INF("Configuring FAW settings");

        FSP_CRITICAL_SECTION_DEFINE;
        FSP_CRITICAL_SECTION_ENTER;
        ret_val = R_FLASH_HP_AccessWindowSet(&g_tfm_fsp_flash_ctrl, start_addr, end_addr);

        FSP_CRITICAL_SECTION_EXIT;
        if (ret_val)
        {
            BOOT_LOG_ERR("Failed to set Flash Access Window: 0x%x", ret_val);
        }
    }
}

/* bootloader platform-specific HW intialization */
int32_t boot_platform_init (void)
{
    int32_t      result;
    iak_data_t * p_initial_attestation = (iak_data_t *) BOOT_TFM_SHARED_IAK_BASE;
    huk_data_t * p_huk                 = (huk_data_t *) BOOT_TFM_SHARED_HUK_BASE;

    result = FLASH_DEV_NAME.Initialize(NULL);
    if (ARM_DRIVER_OK != result)
    {
        return result;
    }

    /* This line is necessary to prevent the compiler from optimizing out this area */
    memset(g_dummy_reserve_for_tfm, 0x0, BOOT_TFM_SHARED_DATA_TOTAL_SIZE);

    /* Set the FAW to lock the Secure code and data region */

    // flash_FAW_Set(FAW_START_ADDR, FAW_END_ADDR);

    result = mbedtls_platform_setup(NULL);
    if (result != 0)
    {
        return result;
    }

    result = psa_crypto_init();
    if (PSA_SUCCESS == result)
    {
        result = psa_generate_random((uint8_t *) BOOT_TFM_SHARED_SEED_BASE, BOOT_TFM_SHARED_SEED_SIZE);
    }

    /* Copy the IAK and HUK from bootloader ROM into the shared RAM area for the application */
    if (PSA_SUCCESS == result)
    {
        memcpy((uint8_t *) p_huk, (uint8_t *) &huk_data, sizeof(huk_data));
        memcpy((uint8_t *) p_initial_attestation, (uint8_t *) &iak_data, sizeof(iak_data));
    }

    return result;
}

void boot_platform_quit (struct boot_arm_vector_table * vt)
{
    /* Clang at O0, stores variables on the stack with SP relative addressing.
     * When manually set the SP then the place of reset vector is lost.
     * Static variables are stored in 'data' or 'bss' section, change of SP has
     * no effect on them.
     */
    static struct boot_arm_vector_table * vt_cpy;

    vt_cpy = vt;
#if BSP_FEATURE_BSP_HAS_SP_MON

    /* Disable MSP monitoring  */
    R_MPU_SPMON->SP[0].CTL = 0;
#endif
#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)

    /* Restore the Main Stack Pointer Limit register's reset value
     * before passing execution to runtime firmware to make the
     * bootloader transparent to it.
     */
    __set_MSPLIM(0);
#endif
    __set_MSP(vt->msp);
    __DSB();
    __ISB();

    boot_jump_to_next_image(vt_cpy->reset);
}
