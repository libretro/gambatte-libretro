#ifndef LCDDEF_H
#define LCDDEF_H

namespace gambatte
{

   enum
   {
      LCDC_BGEN = 0x01,
      LCDC_OBJEN = 0x02,
      LCDC_OBJ2X = 0x04,
      LCDC_TDSEL = 0x10,
      LCDC_WE = 0x20,
      LCDC_EN = 0x80
   };

   enum
   {
      LCDSTAT_LYCFLAG = 0x04,
      LCDSTAT_M0IRQEN = 0x08,
      LCDSTAT_M1IRQEN = 0x10,
      LCDSTAT_M2IRQEN = 0x20,
      LCDSTAT_LYCIRQEN = 0x40
   };

}

#endif
