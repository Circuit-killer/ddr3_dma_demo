`define SYNTHESIS

module ddr3_dma_demo (
    input          OSC_50_B3B,
    input          RESET_n,

    output [14:0]  DDR3_A,
    output [2:0]   DDR3_BA,
    output         DDR3_CAS_n,
    output         DDR3_CKE,
    output         DDR3_CK_n,
    output         DDR3_CK_p,
    output         DDR3_CS_n,
    output [3:0]   DDR3_DM,
    inout  [31:0]  DDR3_DQ,
    inout  [3:0]   DDR3_DQS_n,
    inout  [3:0]   DDR3_DQS_p,
    output         DDR3_ODT,
    output         DDR3_RAS_n,
    output         DDR3_RESET_n,
    input          DDR3_RZQ,
    output         DDR3_WE_n,

    output [14:0] HPS_DDR3_A,
    output [2:0]  HPS_DDR3_BA,
    output        HPS_DDR3_CK_p,
    output        HPS_DDR3_CK_n,
    output        HPS_DDR3_CKE,
    output        HPS_DDR3_CS_n,
    output        HPS_DDR3_RAS_n,
    output        HPS_DDR3_CAS_n,
    output        HPS_DDR3_WE_n,
    output        HPS_DDR3_RESET_n,
    inout  [39:0] HPS_DDR3_DQ,
    inout  [4:0]  HPS_DDR3_DQS_p,
    inout  [4:0]  HPS_DDR3_DQS_n,
    output        HPS_DDR3_ODT,
    output [4:0]  HPS_DDR3_DM,
    input         HPS_DDR3_RZQ,

    output [3:0]   LED
);

wire main_clk = OSC_50_B3B;

soc_system soc (
    .clk_clk (main_clk),
    .reset_reset_n (RESET_n),

    .memory_mem_a (DDR3_A),
    .memory_mem_ba (DDR3_BA),
    .memory_mem_ck (DDR3_CK_p),
    .memory_mem_ck_n (DDR3_CK_n),
    .memory_mem_cke (DDR3_CKE),
    .memory_mem_cs_n (DDR3_CS_n),
    .memory_mem_dm (DDR3_DM),
    .memory_mem_ras_n (DDR3_RAS_n),
    .memory_mem_cas_n (DDR3_CAS_n),
    .memory_mem_we_n (DDR3_WE_n),
    .memory_mem_reset_n (DDR3_RESET_n),
    .memory_mem_dq (DDR3_DQ),
    .memory_mem_dqs (DDR3_DQS_p),
    .memory_mem_dqs_n (DDR3_DQS_n),
    .memory_mem_odt (DDR3_ODT),
    .memory_oct_rzqin (DDR3_RZQ),

    .hps_memory_mem_a (HPS_DDR3_A),
    .hps_memory_mem_ba (HPS_DDR3_BA),
    .hps_memory_mem_ck (HPS_DDR3_CK_p),
    .hps_memory_mem_ck_n (HPS_DDR3_CK_n),
    .hps_memory_mem_cke (HPS_DDR3_CKE),
    .hps_memory_mem_cs_n (HPS_DDR3_CS_n),
    .hps_memory_mem_dm (HPS_DDR3_DM),
    .hps_memory_mem_ras_n (HPS_DDR3_RAS_n),
    .hps_memory_mem_cas_n (HPS_DDR3_CAS_n),
    .hps_memory_mem_we_n (HPS_DDR3_WE_n),
    .hps_memory_mem_reset_n (HPS_DDR3_RESET_n),
    .hps_memory_mem_dq (HPS_DDR3_DQ),
    .hps_memory_mem_dqs (HPS_DDR3_DQS_p),
    .hps_memory_mem_dqs_n (HPS_DDR3_DQS_n),
    .hps_memory_mem_odt (HPS_DDR3_ODT),
    .hps_memory_oct_rzqin (HPS_DDR3_RZQ),
);

endmodule
