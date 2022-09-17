#include "os.h"

void page_table_update(uint32_t pt, uint32_t vpn, uint32_t ppn){
    uint32_t* PT = phys_to_virt(pt<<12);
    uint32_t VPN1 = (vpn >> 10) & 0x3ff;
    uint32_t VPN2 = vpn & 0x3ff;
    if ((PT[VPN1] & 0x1) == 0){
        if (ppn == NO_MAPPING)
            return;
        else
            PT[VPN1] =(alloc_page_frame()<<12)+1;
    }
    uint32_t* p = phys_to_virt(PT[VPN1] & 0xfffffffe);
    if(ppn != NO_MAPPING){
        ppn = (ppn << 12) + 1;
        p[VPN2]=ppn;
    }
    else
        p[VPN2]= (ppn & 0x0);

}

uint32_t page_table_query(uint32_t pt, uint32_t vpn) {
    uint32_t *PT = phys_to_virt(pt<<12);
    uint32_t VPN1 = (vpn >> 10) & 0x3ff;
    uint32_t VPN2 = vpn & 0x3ff;
    if ((PT[VPN1] & 0x1) == 0){
        return NO_MAPPING;}
    uint32_t *p = phys_to_virt(PT[VPN1] & 0xfffffffe);
    if ((p[VPN2]  & 0x1) == 0){
        return NO_MAPPING;}
    return (p[VPN2]>>12);
}