
EXTERN_C_START

#define H2C_CHANNEL_IDENTIFIER				(0x0 >> 2)
#define H2C_CHANNEL_CONTROL1				(0x4 >> 2)
#define H2C_CHANNEL_CONTROL2				(0x8 >> 2)
#define H2C_CHANNEL_CONTROL3				(0xC >> 2)
#define H2C_CHANNEL_STATUS1					(0x40 >> 2)
#define H2C_CHANNEL_STATUS2					(0x44 >> 2)
#define H2C_CHANNEL_COMPLETED_DESC_COUNT	(0x48 >> 2)
#define H2C_CHANNEL_ALIGNMENTS				(0x4C >> 2)
#define H2C_CHANNEL_POLL_MODE_WB_ADDR_LO	(0x88 >> 2)
#define H2C_CHANNEL_POLL_MODE_WB_ADDR_HI	(0x8C >> 2)
#define H2C_CHANNEL_INTERRUPT_ENABLE_MASK1	(0x90 >> 2)
#define H2C_CHANNEL_INTERRUPT_ENABLE_MASK2	(0x94 >> 2)
#define H2C_CHANNEL_INTERRUPT_ENABLE_MASK3	(0x98 >> 2)
#define H2C_CHANNEL_PERF_MON_CONTROL		(0xC0 >> 2)
#define H2C_CHANNEL_PERF_CYCLE_COUNT1		(0xC4 >> 2)
#define H2C_CHANNEL_PERF_CYCLE_COUNT2		(0xC8 >> 2)
#define H2C_CHANNEL_PERF_DATA_COUNT1		(0xCC >> 2)
#define H2C_CHANNEL_PERF_DATA_COUNT2		(0xD0 >> 2)

#define C2H_CHANNEL_IDENTIFIER				(0x0 >> 2)
#define C2H_CHANNEL_CONTROL1				(0x4 >> 2)
#define C2H_CHANNEL_CONTROL2				(0x8 >> 2)
#define C2H_CHANNEL_CONTROL3				(0xC >> 2)
#define C2H_CHANNEL_STATUS1					(0x40 >> 2)
#define C2H_CHANNEL_STATUS2					(0x44 >> 2)
#define C2H_CHANNEL_COMPLETED_DESC_COUNT	(0x48 >> 2)
#define C2H_CHANNEL_ALIGNMENTS				(0x4C >> 2)
#define C2H_CHANNEL_POLL_MODE_WB_ADDR_LO	(0x88 >> 2)
#define C2H_CHANNEL_POLL_MODE_WB_ADDR_HI	(0x8C >> 2)
#define C2H_CHANNEL_INTERRUPT_ENABLE_MASK1	(0x90 >> 2)
#define C2H_CHANNEL_INTERRUPT_ENABLE_MASK2	(0x94 >> 2)
#define C2H_CHANNEL_INTERRUPT_ENABLE_MASK3	(0x98 >> 2)
#define C2H_CHANNEL_PERF_MON_CONTROL		(0xC0 >> 2)
#define C2H_CHANNEL_PERF_CYCLE_COUNT1		(0xC4 >> 2)
#define C2H_CHANNEL_PERF_CYCLE_COUNT2		(0xC8 >> 2)
#define C2H_CHANNEL_PERF_DATA_COUNT1		(0xCC >> 2)
#define C2H_CHANNEL_PERF_DATA_COUNT2		(0xD0 >> 2)

#define IRQ_BLOCK_IDENTIFIER				(0x0 >> 2)
#define IRQ_BLOCK_USER_INT_ENABLE_MASK1		(0x4 >> 2)
#define IRQ_BLOCK_USER_INT_ENABLE_MASK2		(0x8 >> 2)
#define IRQ_BLOCK_USER_INT_ENABLE_MASK3		(0xC >> 2)
#define IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK1	(0x10 >> 2)
#define IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK2	(0x14 >> 2)
#define IRQ_BLOCK_CHANNEL_INT_ENABLE_MASK3	(0x18 >> 2)
#define IRQ_BLOCK_USER_INT_REQUEST			(0x40 >> 2)
#define IRQ_BLOCK_CHANNEL_INT_REQUEST		(0x44 >> 2)
#define IRQ_BLOCK_USER_INT_PENDING			(0x48 >> 2)
#define IRQ_BLOCK_CHANNEL_INT_PENDING		(0x4C >> 2)
#define IRQ_BLOCK_USER_VECTOR_NUMBER1		(0x80 >> 2)
#define IRQ_BLOCK_USER_VECTOR_NUMBER2		(0x84 >> 2)
#define IRQ_BLOCK_USER_VECTOR_NUMBER3		(0x88 >> 2)
#define IRQ_BLOCK_USER_VECTOR_NUMBER4		(0x9C >> 2)
#define IRQ_BLOCK_CHANNEL_VECTOR_NUMBER1	(0xA0 >> 2)
#define IRQ_BLOCK_CHANNEL_VECTOR_NUMBER2	(0xA4 >> 2)


#define CONFIG_BLOCK_IDENTIFIER					(0x0 >> 2)
#define CONFIG_BLOCK_BUSDEV						(0x4 >> 2)
#define CONFIG_BLOCK_PCIE_MAX_PAYLOAD			(0x8 >> 2)
#define CONFIG_BLOCK_PCIE_MAX_READ_REQ_SIZE		(0xC >> 2)
#define CONFIG_BLOCK_SYSTEM_ID					(0x10 >> 2)
#define CONFIG_BLOCK_MSI_ENABLE					(0x14 >> 2)
#define CONFIG_BLOCK_PCIE_DATA_WIDTH			(0x18 >> 2)
#define CONFIG_BLOCK_PCIE_CONTROL				(0x1C >> 2)
#define CONFIG_BLOCK_AXI_USER_MAX_PAYLOAD_SIZE	(0x40 >> 2)
#define CONFIG_BLOCK_AXI_USER_MAX_READ_REQ_SIZE	(0x44 >> 2)
#define CONFIG_BLOCK_WRITE_FLUSH_TIMEOUT		(0x60 >> 2)


#define H2C_SGDMA_IDENTIFIER					(0x0 >> 2)
#define H2C_SGDMA_DESCRIPTOR_ADDRESS_LO			(0x80 >> 2)
#define H2C_SGDMA_DESCRIPTOR_ADDRESS_HI			(0x84 >> 2)
#define H2C_SGDMA_DESCRIPTOR_ADJACENT			(0x88 >> 2)
#define H2C_SGDMA_DESCRIPTOR_CREDITS			(0x8C >> 2)

#define C2H_SGDMA_IDENTIFIER					(0x0 >> 2)
#define C2H_SGDMA_DESCRIPTOR_ADDRESS_LO			(0x80 >> 2)
#define C2H_SGDMA_DESCRIPTOR_ADDRESS_HI			(0x84 >> 2)
#define C2H_SGDMA_DESCRIPTOR_ADJACENT			(0x88 >> 2)
#define C2H_SGDMA_DESCRIPTOR_CREDITS			(0x8C >> 2)

#define SGDMA_IDENTIFIER						(0x0 >> 2)
#define SGDMA_DESCRIPTOR_CONTROL1				(0x10 >> 2)
#define SGDMA_DESCRIPTOR_CONTROL2				(0x14 >> 2)
#define SGDMA_DESCRIPTOR_CONTROL3				(0x18 >> 2)
#define SGDMA_DESCRIPTOR_CREDIT_MODE_ENABLE		(0x20 >> 2)
#define SG_DESCRIPTOR_MODE_ENABLE1				(0x24 >> 2)
#define SG_DESCRIPTOR_MODE_ENABLE2				(0x28 >> 2)

#define MSIX_VECTOR0_ADDRESS_LO					(0x0 >> 2)
#define MSIX_VECTOR0_ADDRESS_HI					(0x4 >> 2)
#define MSIX_VECTOR0_DATA						(0x8 >> 2)
#define MSIX_VECTOR0_CONTROL					(0xC >> 2)
#define MSIX_VECTOR31_ADDRESS_LO				(0x1F0 >> 2)
#define MSIX_VECTOR31_ADDRESS_HI				(0x1F4 >> 2)
#define MSIX_VECTOR31_DATA						(0x1F8 >> 2)
#define MSIX_VECTOR31_CONTROL					(0x1FC >> 2)
#define MSIX_PENDING_BIT_ARRAY					(0xFE0 >> 2)

#pragma pack(push, 1)
typedef struct 
{
	UCHAR busy : 1;
	UCHAR descriptor_stopped : 1;
	UCHAR descriptor_completed : 1;
	UCHAR align_mismatch : 1;
	UCHAR magic_stopped : 1;
	UCHAR invalid_length : 1;
	UCHAR idle_stopped : 1;
	UCHAR : 2;
	UCHAR read_error : 5;
	UCHAR write_error : 5;
	UCHAR descr_error : 5;
	UCHAR : 8;
} H2C_channel_status;

#define XDMA_VERSION_2015_4 1
#define XDMA_VERSION_2016_1 2
#define XDMA_VERSION_2016_2 3
#define XDMA_VERSION_2016_3 4
#define XDMA_VERSION_2016_4 5
#define XDMA_VERSION_2017_1 6

typedef struct
{
	UCHAR version;
	UCHAR id_target : 4;
	UCHAR : 3;
	UCHAR stream : 1;
	UCHAR channel_target : 4;
	USHORT subsystem_identifier : 12; // 0x1fc
} H2C_channel_identifier;

typedef struct
{
	UCHAR run : 1;
	UCHAR ie_descriptor_stopped : 1;
	UCHAR ie_descriptor_completed : 1;
	UCHAR ie_align_mismatch : 1;
	UCHAR ie_magic_stopped : 1;
	UCHAR ie_invalid_length : 1;
	UCHAR ie_idle_stopped : 1;
	UCHAR : 2;
	UCHAR ie_read_error : 5;
	UCHAR ie_write_error : 5;
	UCHAR ie_desc_error : 5;
	UCHAR : 1;
	UCHAR non_inc_mode : 1;
	UCHAR pollmode_wb_enable : 1;
	UCHAR c2h_stream_wb : 1;
	UCHAR : 4;
} H2C_channel_control;

typedef struct
{
	UCHAR address_bits;
	UCHAR len_granularity;
	UCHAR addr_alignment;
	UCHAR : 8;
} H2C_channel_alignments;

typedef struct
{
	UCHAR : 1;
	UCHAR im_descriptor_stopped : 1;
	UCHAR im_descriptor_completed : 1;
	UCHAR im_align_mismatch : 1;
	UCHAR im_magic_stopped : 1;
	UCHAR im_invalid_length : 1;
	UCHAR im_idle_stopped : 1;
	UCHAR : 2;
	UCHAR m_read_error : 5;
	UCHAR im_write_error : 5;
	UCHAR im_desc_error : 5;
	UCHAR : 8;
} H2C_channel_interrupt_enable_mask;

typedef struct
{
	UCHAR auto_stop : 1;
	UCHAR clear : 1;
	UCHAR run : 1;
	UCHAR : 5;
	ULONG : 24;
} H2C_channel_perfmon_control;

typedef struct
{
	H2C_channel_identifier				identifier;
	H2C_channel_control					control;
	H2C_channel_control					control_w1s;
	H2C_channel_control					control_w1c;
	ULONG gap[12];
	H2C_channel_status					status;
	H2C_channel_status					status_rc;
	ULONG								compl_descriptor_count;
	H2C_channel_alignments				alignments;
	ULONG gap2[14];
	PHYSICAL_ADDRESS					pollmode_wb_addr;
	H2C_channel_interrupt_enable_mask	interrupt_enable;
	H2C_channel_interrupt_enable_mask	interrupt_enable_w1s;
	H2C_channel_interrupt_enable_mask	interrupt_enable_w1c;
	ULONG gap3[9];
	H2C_channel_perfmon_control			performance_monitor_control;
	ULONG								performance_cycle_count_lo;
	ULONG								performance_cycle_count_hi;
	ULONG								performance_data_count_lo;
	ULONG								performance_data_count_hi;
} H2C_channel;
#pragma pack(pop)

#define CHANNEL_STATUS_BUSY				0x1
#define CHANNEL_STATUS_STOPPED			0x2
#define CHANNEL_STATUS_COMPLETED		0x4
#define CHANNEL_STATUS_ALIGN_MISMATCH	0x8
#define CHANNEL_STATUS_MAGIC_STOPPED	0x10
#define CHANNEL_STATUS_INVALID_LENGTH	0x20
#define CHANNEL_STATUS_IDLE_STOPPED		0x40

EXTERN_C_END
