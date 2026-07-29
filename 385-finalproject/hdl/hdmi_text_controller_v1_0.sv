//Provided HDMI_Text_controller_v1_0 for HDMI AXI4 IP 
//Fall 2024 Distribution

//Modified 3/10/24 by Zuofu
//Updated 11/18/24 by Zuofu


`timescale 1 ns / 1 ps

module hdmi_text_controller_v1_0 #
(
    // Parameters of Axi Slave Bus Interface S00_AXI
    // Modify parameters as necessary for access of full VRAM range

    parameter integer C_AXI_DATA_WIDTH	= 32,
    parameter integer C_AXI_ADDR_WIDTH	= 14 
)
(
    // Users to add ports here

    output logic hdmi_clk_n,
    output logic hdmi_clk_p,
    output logic [2:0] hdmi_tx_n,
    output logic [2:0] hdmi_tx_p,

    // User ports ends
    // Do not modify the ports beyond this line


    // Ports of Axi Slave Bus Interface AXI
    input logic  axi_aclk,
    input logic  axi_aresetn,
    input logic [C_AXI_ADDR_WIDTH-1 : 0] axi_awaddr,
    input logic [2 : 0] axi_awprot,
    input logic  axi_awvalid,
    output logic  axi_awready,
    input logic [C_AXI_DATA_WIDTH-1 : 0] axi_wdata,
    input logic [(C_AXI_DATA_WIDTH/8)-1 : 0] axi_wstrb,
    input logic  axi_wvalid,
    output logic  axi_wready,
    output logic [1 : 0] axi_bresp,
    output logic  axi_bvalid,
    input logic  axi_bready,
    input logic [C_AXI_ADDR_WIDTH-1 : 0] axi_araddr,
    input logic [2 : 0] axi_arprot,
    input logic  axi_arvalid,
    output logic  axi_arready,
    output logic [C_AXI_DATA_WIDTH-1 : 0] axi_rdata,
    output logic [1 : 0] axi_rresp,
    output logic  axi_rvalid,
    input logic  axi_rready
);

//additional logic variables as necessary to support VGA, and HDMI modules.
    logic [31:0] keycode0_gpio, keycode1_gpio;
    logic clk_25MHz, clk_125MHz, clk, clk_100MHz;
    logic locked;
    logic [9:0] drawX, drawY;
    logic hsync, vsync, vde;
    logic [3:0] red, green, blue;
    logic reset_ah;
    
    
// Instantiation of Axi Bus Interface AXI
hdmi_text_controller_v1_0_AXI # ( 
    .C_S_AXI_DATA_WIDTH(C_AXI_DATA_WIDTH),
    .C_S_AXI_ADDR_WIDTH(C_AXI_ADDR_WIDTH)
) hdmi_text_controller_v1_0_AXI_inst (
    .S_AXI_ACLK(axi_aclk),
    .S_AXI_ARESETN(axi_aresetn),
    .S_AXI_AWADDR(axi_awaddr),
    .S_AXI_AWPROT(axi_awprot),
    .S_AXI_AWVALID(axi_awvalid),
    .S_AXI_AWREADY(axi_awready),
    .S_AXI_WDATA(axi_wdata),
    .S_AXI_WSTRB(axi_wstrb),
    .S_AXI_WVALID(axi_wvalid),
    .S_AXI_WREADY(axi_wready),
    .S_AXI_BRESP(axi_bresp),
    .S_AXI_BVALID(axi_bvalid),
    .S_AXI_BREADY(axi_bready),
    .S_AXI_ARADDR(axi_araddr),
    .S_AXI_ARPROT(axi_arprot),
    .S_AXI_ARVALID(axi_arvalid),
    .S_AXI_ARREADY(axi_arready),
    .S_AXI_RDATA(axi_rdata),
    .S_AXI_RRESP(axi_rresp),
    .S_AXI_RVALID(axi_rvalid),
    .S_AXI_RREADY(axi_rready),
    .slave_reg_idx(slave_reg_idx),
    .slave_reg_out(slave_reg_out),
    .vsync(vsync),
    .drawX(drawX),
    .drawY(drawY),
     .color_reg_one(color_register_one),
    .color_reg_two(color_register_two),
    .color_reg_three(color_register_three),
    .color_reg_four(color_register_four),
    .color_reg_five(color_register_five),
    .color_reg_six(color_register_six),
    .color_reg_seven(color_register_seven),
    .color_reg_eight(color_register_eight),
    .color_reg_nine(color_register_nine),
    .color_reg_ten(color_register_ten),
    .color_reg_eleven(color_register_eleven),
    .color_reg_twelve(color_register_twelve),
    .color_reg_thirteen(color_register_thirteen),
    .color_reg_fourteen(color_register_fourteen),
    .color_reg_fifteen(color_register_fifteen),
    .color_reg_sixteen(color_register_sixteen),
    .clk_25MHz(clk_25MHz)
);

logic [31:0] slave_reg_out;
//Instiante clocking wizard, VGA sync generator modules, and VGA-HDMI IP here. For a hint, refer to the provided
//top-level from the previous lab. You should get the IP to generate a valid HDMI signal (e.g. blue screen or gradient)
//prior to working on the text drawing.
    //clock wizard configured with a 1x and 5x clock for HDMI
    clk_wiz_0 clk_wiz (
        .clk_out1(clk_25MHz),
        .clk_out2(clk_125MHz),
        .reset(1'b0),//check
        .locked(locked),
        .clk_in1(axi_aclk) //NEEDS TO BE CHECKED
    );
hdmi_tx_0 vga_to_hdmi (
        //Clocking and Reset
        .pix_clk(clk_25MHz),
        .pix_clkx5(clk_125MHz),
        .pix_clk_locked(locked),
        .rst(~axi_aresetn),
        //Color and Sync Signals
        .red(red),
        .green(green),
        .blue(blue),
        .hsync(hsync),
        .vsync(vsync),
        .vde(vde),
        
        //aux Data (unused)
        .aux0_din(4'b0),
        .aux1_din(4'b0),
        .aux2_din(4'b0),
        .ade(1'b0),
        
        //Differential outputs
        .TMDS_CLK_P(hdmi_clk_p),          
        .TMDS_CLK_N(hdmi_clk_n),          
        .TMDS_DATA_P(hdmi_tx_p),         
        .TMDS_DATA_N(hdmi_tx_n)          
    );
    vga_controller vga (
        .pixel_clk(clk_25MHz),
        .reset(~axi_aresetn),//check all of these, ax_reset or inverted for active low?
        .hs(hsync),
        .vs(vsync),
        .sync(),
        .active_nblank(vde),
        .drawX(drawX),
        .drawY(drawY)
    );   

logic [10:0] font_addr;
logic [7:0] font_data;
font_rom font_rom_inst(
    .addr(font_addr),
    .data(font_data)
);




logic [11:0] color_register_one;
logic [11:0] color_register_two;
logic [11:0] color_register_three;
logic [11:0] color_register_four;
logic [11:0] color_register_five;
logic [11:0] color_register_six;
logic [11:0] color_register_seven;
logic [11:0] color_register_eight;
logic [11:0] color_register_nine;
logic [11:0] color_register_ten;
logic [11:0] color_register_eleven;
logic [11:0] color_register_twelve;
logic [11:0] color_register_thirteen;
logic [11:0] color_register_fourteen;
logic [11:0] color_register_fifteen;
logic [11:0] color_register_sixteen;



logic [6:0]  column_text;
logic [4:0]  row_text;
logic [10:0] slave_reg_idx;
logic [31:0] our_current_data;

logic        inverse;
logic [6:0]  our_code;
logic [3:0]  foreg_idx, backg_idx;


logic [2:0] x_bit;
logic [2:0] delay_x_bit;
logic       delay_char;
logic       delay_vd;

logic [11:0] foreg_color, backg_color;
logic [11:0] foreg_final, backg_final;

logic [7:0]  our_row_font;
logic        glyph;


logic [11:0] pixel_color;




logic [11:0] palette [0:15];
assign palette[0]  = color_register_one;
assign palette[1]  = color_register_two;
assign palette[2]  = color_register_three;
assign palette[3]  = color_register_four;
assign palette[4]  = color_register_five;
assign palette[5]  = color_register_six;
assign palette[6]  = color_register_seven;
assign palette[7]  = color_register_eight;
assign palette[8]  = color_register_nine;
assign palette[9]  = color_register_ten;
assign palette[10] = color_register_eleven;
assign palette[11] = color_register_twelve;
assign palette[12] = color_register_thirteen;
assign palette[13] = color_register_fourteen;
assign palette[14] = color_register_fifteen;
assign palette[15] = color_register_sixteen;

assign column_text   = drawX[9:3];
assign row_text      = drawY[8:4];
assign slave_reg_idx = (row_text * 11'd40) + (column_text >> 1);
assign our_current_data  = slave_reg_out;

assign x_bit = 3'd7 - drawX[2:0];

// only delay the signals needed to align with 1-cycle BRAM output
always_ff @(posedge clk_25MHz) begin
    delay_x_bit    <= x_bit;
    delay_char <= column_text[0];
    delay_vd      <= vde;
end

always_comb begin
    if (delay_char == 1'b0) begin
        inverse    = our_current_data[15];
        our_code   = our_current_data[14:8];
        foreg_idx = our_current_data[7:4];
        backg_idx = our_current_data[3:0];
    end
    else begin
        inverse    = our_current_data[31];
        our_code   = our_current_data[30:24];
        foreg_idx = our_current_data[23:20];
        backg_idx = our_current_data[19:16];
    end
end

assign font_addr = {our_code, drawY[3:0]};
assign our_row_font  = font_data;
assign glyph     = our_row_font[delay_x_bit];

assign foreg_color = palette[foreg_idx];
assign backg_color = palette[backg_idx];

always_comb begin
    if (inverse) begin
        foreg_final = backg_color;
        backg_final = foreg_color;
    end
    else begin
        foreg_final = foreg_color;
        backg_final = backg_color;
    end

    if (delay_vd) begin
        if (glyph)
            pixel_color = foreg_final;
        else
            pixel_color = backg_final;
    end
    else begin
        pixel_color = 12'h000;
    end
end

assign red   = pixel_color[11:8];
assign green = pixel_color[7:4];
assign blue  = pixel_color[3:0];


endmodule

module colormapper ( output logic [3:0] Red, Green, Blue);
    always_comb
    begin: RGB_Display
            Red = 4'h0;
            Green = 4'hf;
            Blue = 4'h0;
    end 
endmodule

