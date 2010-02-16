
#include <stdlib.h>
#include <mrfCommonIO.h>
#include <devLibPCI.h>

#include <iocsh.h>
#include <epicsExport.h>

#include "mrmpci.h"

#include "evrRegMap.h"
#include "plx9030.h"

/*
 * This file contains a single stand-alone diagnositic routine
 * to probe and report the status of all MRM EVRs in the system.
 */

static const epicsPCIID mrmevrs[] = {
   DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030, PCI_VENDOR_ID_PLX,
                              DEVPCI_ANY_SUBDEVICE,   PCI_VENDOR_ID_MRF)
  ,DEVPCI_END
};

static
const
struct printreg
{
  char label[10];
  epicsUInt32 offset;
  int rsize;
  int verb;
} printreg[] = {
#define REGINFO(label, name, size, verb) {label, U##size##_##name, size, verb}
REGINFO("Version", FWVersion, 32, 0),
REGINFO("Control", Control, 32, 1),
REGINFO("Status",  Status, 32, 1),
REGINFO("IRQ Flag",IRQFlag, 32, 1),
REGINFO("IRQ Ena", IRQEnable, 32, 1),
REGINFO("IRQPlsmap",IRQPulseMap, 16, 2),
REGINFO("DBufCtrl",DataBufCtrl, 32, 2),
REGINFO("DBufTxCt",DataTxCtrl, 32, 2),
REGINFO("CountPS", CounterPS, 32, 2),
REGINFO("USecDiv", USecDiv, 32, 2),
REGINFO("ClkCtrl", ClkCtrl, 32, 2),
REGINFO("LogSts",  LogStatus, 32, 2),
REGINFO("TSSec",TSSec, 32, 5),
REGINFO("TSEvt",TSEvt, 32, 5),
REGINFO("TSSecLath",TSSecLatch, 32, 5),
REGINFO("TSEvtLath",TSEvtLatch, 32, 5),
REGINFO("FracDiv", FracDiv, 32, 2),
REGINFO("Scaler0",Scaler(0),32,5),
REGINFO("Pul0Ctrl",PulserCtrl(0),32,5),
REGINFO("Pul0Scal",PulserScal(0),32,5),
REGINFO("Pul0Dely",PulserDely(0),32,5),
REGINFO("Pul0Wdth",PulserWdth(0),32,5),
REGINFO("FP0MAP",OutputMapFP(0),16,5),
REGINFO("FPU0MAP",OutputMapFPUniv(0),16,5),
REGINFO("RB0MAP",OutputMapRB(0),16,5),
REGINFO("FPIN0CFG",InputMapFPCfg(0),8,5),
REGINFO("FPIN0DBs",InputMapFPDBus(0),8,5),
REGINFO("FPIN0Bck",InputMapFPBEvt(0),8,5),
REGINFO("FPIN0Ext",InputMapFPEEvt(0),8,5)
#undef REGINFO
};

#define NELEM(X) (sizeof(X)/sizeof(X[0]))

struct evrshow
{
  int verb;
};

static
int
evrshow(void* arg,epicsPCIDevice *cur)
{
  size_t reg,i;
  epicsUInt32 blen;
  volatile void *base;
  volatile epicsUInt8 *plx=0, *evr=0;
  struct evrshow *priv=arg;
  int verb=priv->verb;

  printf("Device %u:%u.%u\n",cur->bus,cur->device,cur->function);

  if(verb>0){
    printf("Found %04x:%04x %04x:%04x\n",cur->id.vendor,cur->id.device,
                                         cur->id.sub_vendor,cur->id.sub_device);
    printf("IRQ %u\n",cur->irq);
  }

  for(i=0;i<PCIBARCOUNT;i++){
    if(devPCIToLocalAddr(cur,i,&base,0)){
      if(verb>4)
        printf("Failed to map BAR %u\n",i);
      continue;
    }
    switch(i){
    case 0: plx=base;break;
    case 2: evr=base;break;
    }
    if(verb<=2)
      continue;

    printf("Bar %u ",i);
    if(cur->bar[i].ioport){
      printf("IO\n");
    }else{
      printf("MEM %s-bit%s\n",
          cur->bar[i].addr64 ? "64":"32",
          cur->bar[i].below1M? " below 1M":"");
    }
    printf("Addr: %08x\n",(epicsUInt32)base);
    blen=devPCIBarLen(cur,i);
    printf("Length: %08x\n",blen);
  }

  if(!!plx){
/* Swap access little endian systems so we don't have no worry about
 * byte order :)
 */
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
    BITSET(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
    BITCLR(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
#endif
    blen=LE_READ32(plx,LAS0BRD);
    if(verb>=2){
      printf("PLX range 0 config: 0x%08x\n",blen);
      printf("PLX INTCSR %04x\n",LE_READ16(plx, INTCSR));
    }
  }

  if(!!evr){
    printf("EVR\n");
    for(reg=0; reg<NELEM(printreg); reg++){
      if(printreg[reg].verb > verb)
        continue;

      switch(printreg[reg].rsize){
      case 8:
        printf("%9s: %02x\n",
                printreg[reg].label,
                ioread8(evr+printreg[reg].offset));
        break;
      case 16:
        printf("%9s: %04x\n",
               printreg[reg].label,
               nat_ioread16(evr+printreg[reg].offset));
        break;
      case 32:
        printf("%9s: %08x\n",
               printreg[reg].label,
               nat_ioread32(evr+printreg[reg].offset));
        break;
      }
    }

  }

  return 0;
}

void
mrmevrdump(int verb)
{
  int err;
  struct evrshow p;

  p.verb=verb;

  printf("Searching for PCI EVRs\n");

  err=devPCIFindCB(mrmevrs,&evrshow,&p,0);
  if(!err){
    printf("Search error: %d\n",err);
  }else if(verb>=1){
    printf("Done.\n");
  }
}

static const iocshArg mrmevrdumpArg0 = { "Verbosity",iocshArgInt};
static const iocshArg * const mrmevrdumpArgs[1] =
{
    &mrmevrdumpArg0
};
static const iocshFuncDef mrmevrdumpFuncDef =
    {"mrmevrdump",1,mrmevrdumpArgs};
static void mrmevrdumpCallFunc(const iocshArgBuf *args)
{
    mrmevrdump(args[0].ival);
}

void mrmsearchreg()
{
  iocshRegister(&mrmevrdumpFuncDef,mrmevrdumpCallFunc);
}

epicsExportRegistrar(mrmsearchreg);
