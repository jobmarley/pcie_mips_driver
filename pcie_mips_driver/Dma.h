

EXTERN_C_START

BOOLEAN pciemipsdriverInterruptISR(_In_ WDFINTERRUPT Interrupt, _In_ ULONG MessageID);
VOID pciemipsdriverInterruptDPC(_In_ WDFINTERRUPT Interrupt, _In_ WDFOBJECT AssociatedObject);
BOOLEAN
pciemipsdriverEvtProgramDMA(
    _In_
    WDFDMATRANSACTION Transaction,
    _In_
    WDFDEVICE Device,
    _In_
    WDFCONTEXT Context,
    _In_
    WDF_DMA_DIRECTION Direction,
    _In_
    PSCATTER_GATHER_LIST SgList
);

EXTERN_C_END