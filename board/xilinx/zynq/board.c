/*
 * (C) Copyright 2012 Michal Simek <monstr@monstr.eu>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <netdev.h>
#include <zynqpl.h>
#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>
#include <linux/mtd/mtd.h>
#include <malloc.h>
#include <nand.h>
#define UPGARDE_MARKER_ADDR     0x1040000
#define UPGARDE_MARKER_LEN      16
#define NAND_SIZE_LIMIT         (128*1024)
DECLARE_GLOBAL_DATA_PTR;

#if (defined(CONFIG_FPGA) && !defined(CONFIG_SPL_BUILD)) || \
    (defined(CONFIG_SPL_FPGA_SUPPORT) && defined(CONFIG_SPL_BUILD))
Xilinx_desc fpga;

/* It can be done differently */
Xilinx_desc fpga010 = XILINX_XC7Z010_DESC(0x10);
Xilinx_desc fpga015 = XILINX_XC7Z015_DESC(0x15);
Xilinx_desc fpga020 = XILINX_XC7Z020_DESC(0x20);
Xilinx_desc fpga030 = XILINX_XC7Z030_DESC(0x30);
Xilinx_desc fpga045 = XILINX_XC7Z045_DESC(0x45);
Xilinx_desc fpga100 = XILINX_XC7Z100_DESC(0x100);
#endif

int board_init(void)
{
#if defined(CONFIG_ENV_IS_IN_EEPROM) && !defined(CONFIG_SPL_BUILD)
	unsigned char eepromsel = CONFIG_SYS_I2C_MUX_EEPROM_SEL;
#endif
#if (defined(CONFIG_FPGA) && !defined(CONFIG_SPL_BUILD)) || \
    (defined(CONFIG_SPL_FPGA_SUPPORT) && defined(CONFIG_SPL_BUILD))
	u32 idcode;

	idcode = zynq_slcr_get_idcode();

	switch (idcode) {
	case XILINX_ZYNQ_7010:
		fpga = fpga010;
		break;
	case XILINX_ZYNQ_7015:
		fpga = fpga015;
		break;
	case XILINX_ZYNQ_7020:
		fpga = fpga020;
		break;
	case XILINX_ZYNQ_7030:
		fpga = fpga030;
		break;
	case XILINX_ZYNQ_7045:
		fpga = fpga045;
		break;
	case XILINX_ZYNQ_7100:
		fpga = fpga100;
		break;
	}
#endif

	/* temporary hack to clear pending irqs before Linux as it
	 * will hang Linux
	 */
	writel(0x26d, 0xe0001014);

#if (defined(CONFIG_FPGA) && !defined(CONFIG_SPL_BUILD)) || \
    (defined(CONFIG_SPL_FPGA_SUPPORT) && defined(CONFIG_SPL_BUILD))
	fpga_init();
	fpga_add(fpga_xilinx, &fpga);
#endif
#if defined(CONFIG_ENV_IS_IN_EEPROM) && !defined(CONFIG_SPL_BUILD)
	if (eeprom_write(CONFIG_SYS_I2C_MUX_ADDR, 0, &eepromsel, 1))
		puts("I2C:EEPROM selection failed\n");
#endif
	return 0;
}

int board_late_init(void)
{
    /*Zynq7010 upgrade*/
   nand_info_t *nand = NULL;
   //nand = &nand_info[0];
   unsigned int ret, i;
   size_t read_len;
   unsigned int marker_is_right = 0;
   //char *nandroot = NULL;
    char *bootargs = NULL;
   unsigned char *upgrade_buf = NULL;
    unsigned int gpio_value = 0;

    // check ipsig gpio to choose mount which filesystem
    gpio_value = readl(0xe000a000 + 0x64);
    printk("--- gpio value = 0x%08x\n", gpio_value);
    if(gpio_value & 0x80000)
    {
        printk("--- mount angstrom file system\n");
    }
    else
    {
        setenv("bootargs", "noinitrd mem=1008M console=ttyPS0,115200 root=ubi0:rootfs ubi.mtd=2 rootfstype=ubifs rw rootwait");
        bootargs = getenv("bootargs");
        if(bootargs)
        {   
            //printk("nandroot: %s\n", nandroot);
            printk("bootargs: %s\n", bootargs);
            printk("--- ipsig :  This time is boot for upgrade ---\n");
        }
        else
        {
            //printk("--- setenv nandroot to /dev/mtdblock2 failed! ---\n");
            printk("--- ipsig : setenv bootargs to /dev/mtdblock2 failed! ---\n");
        }
    }


   nand = &nand_info[0];
   upgrade_buf = (unsigned char *)malloc(UPGARDE_MARKER_LEN*sizeof(unsigned char));
   memset(upgrade_buf, 0, UPGARDE_MARKER_LEN);
   read_len = UPGARDE_MARKER_LEN;
   //ret = nand_read_skip_bad(nand, UPGARDE_MARKER_ADDR, &read_len, upgrade_buf);
   ret = nand_read(nand, UPGARDE_MARKER_ADDR, &read_len, upgrade_buf);
   //ret = nand_read_skip_bad(nand, UPGARDE_MARKER_ADDR, &read_len, NULL, NAND_SIZE_LIMIT, upgrade_buf);
   if(!ret)
   {
       for(i=0; i<UPGARDE_MARKER_LEN; i++)
       {
           printk("--- upgrade marker upgrade_buf[%d] = 0x%x ---\n", i, *(upgrade_buf +i));
       }

       for(i=0; i<UPGARDE_MARKER_LEN; i++)
       {
           if(*(upgrade_buf + i) != i)
           {
               break;
           }
           marker_is_right = 1;
       }

       if(marker_is_right)
       {
           //setenv("nandroot", "/dev/mtdblock2");
           //nandroot = getenv("nandroot");
            setenv("bootargs", "noinitrd mem=1008M console=ttyPS0,115200 root=ubi0:rootfs ubi.mtd=2 rootfstype=ubifs rw rootwait");
            bootargs = getenv("bootargs");
           //if(nandroot)
            if(bootargs)
           {
               //printk("nandroot: %s\n", nandroot);
                printk("bootargs: %s\n", bootargs);
               printk("--- This time is boot for upgrade ---\n");
           }
           else
           {
               //printk("--- setenv nandroot to /dev/mtdblock2 failed! ---\n");
                printk("--- setenv bootargs to /dev/mtdblock2 failed! ---\n");
           }
       }
   }
   else
   {
       printk("--- upgrade read nand error! ret = %d ---\n", ret);
   }
   free(upgrade_buf);
   /*Zynq7010 upgrade end*/    

	switch ((zynq_slcr_get_boot_mode()) & ZYNQ_BM_MASK) {
	case ZYNQ_BM_QSPI:
		setenv("modeboot", "qspiboot");
		break;
	case ZYNQ_BM_NAND:
		setenv("modeboot", "nandboot");
		break;
	case ZYNQ_BM_NOR:
		setenv("modeboot", "norboot");
		break;
	case ZYNQ_BM_SD:
		setenv("modeboot", "sdboot");
		break;
	case ZYNQ_BM_JTAG:
		setenv("modeboot", "jtagboot");
		break;
	default:
		setenv("modeboot", "");
		break;
	}

	return 0;
}

int board_eth_init(bd_t *bis)
{
	u32 ret = 0;

#ifdef CONFIG_XILINX_AXIEMAC
	ret |= xilinx_axiemac_initialize(bis, XILINX_AXIEMAC_BASEADDR,
						XILINX_AXIDMA_BASEADDR);
#endif
#ifdef CONFIG_XILINX_EMACLITE
	u32 txpp = 0;
	u32 rxpp = 0;
# ifdef CONFIG_XILINX_EMACLITE_TX_PING_PONG
	txpp = 1;
# endif
# ifdef CONFIG_XILINX_EMACLITE_RX_PING_PONG
	rxpp = 1;
# endif
	ret |= xilinx_emaclite_initialize(bis, XILINX_EMACLITE_BASEADDR,
			txpp, rxpp);
#endif

#if defined(CONFIG_ZYNQ_GEM)
# if defined(CONFIG_ZYNQ_GEM0)
	ret |= zynq_gem_initialize(bis, ZYNQ_GEM_BASEADDR0,
						CONFIG_ZYNQ_GEM_PHY_ADDR0, 0);
# endif
# if defined(CONFIG_ZYNQ_GEM1)
	ret |= zynq_gem_initialize(bis, ZYNQ_GEM_BASEADDR1,
						CONFIG_ZYNQ_GEM_PHY_ADDR1, 0);
# endif
#endif
	return ret;
}

#ifdef CONFIG_CMD_MMC
int board_mmc_init(bd_t *bd)
{
	int ret = 0;

#if defined(CONFIG_ZYNQ_SDHCI)
# if defined(CONFIG_ZYNQ_SDHCI0)
	ret = zynq_sdhci_init(ZYNQ_SDHCI_BASEADDR0);
# endif
# if defined(CONFIG_ZYNQ_SDHCI1)
	ret |= zynq_sdhci_init(ZYNQ_SDHCI_BASEADDR1);
# endif
#endif
	return ret;
}
#endif

int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

	zynq_ddrc_init();

	return 0;
}
