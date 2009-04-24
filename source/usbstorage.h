#ifndef _USBSTORAGE_H_
#define _USBSTORAGE_H_ 

/* Prototypes */
s32  USBStorage_GetCapacity(u32 *);

bool USBStorage_Init(void);
bool USBStorage_Deinit(void);
bool USBStorage_ReadSectors(u32, u32, void *);
bool USBStorage_WriteSectors(u32, u32, void *);

/* Disc interface */
extern const DISC_INTERFACE __io_usb2storage;

#endif
