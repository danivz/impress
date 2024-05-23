library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VComponents.all;

entity mux_ICAP is
  generic (
    -- Width of S_AXI data bus
    C_S00_AXI_DATA_WIDTH  : integer := 32;
    -- Width of S_AXI address bus
    C_S00_AXI_ADDR_WIDTH  : integer := 4
  );
  port (
    -- Ports of Axi Slave Bus Interface S00_AXI
    s00_axi_aclk  : in std_logic;
    s00_axi_aresetn : in std_logic;
    s00_axi_awaddr  : in std_logic_vector(C_S00_AXI_ADDR_WIDTH-1 downto 0);
    s00_axi_awprot  : in std_logic_vector(2 downto 0);
    s00_axi_awvalid : in std_logic;
    s00_axi_awready : out std_logic;
    s00_axi_wdata : in std_logic_vector(C_S00_AXI_DATA_WIDTH-1 downto 0);
    s00_axi_wstrb : in std_logic_vector((C_S00_AXI_DATA_WIDTH/8)-1 downto 0);
    s00_axi_wvalid  : in std_logic;
    s00_axi_wready  : out std_logic;
    s00_axi_bresp : out std_logic_vector(1 downto 0);
    s00_axi_bvalid  : out std_logic;
    s00_axi_bready  : in std_logic;
    s00_axi_araddr  : in std_logic_vector(C_S00_AXI_ADDR_WIDTH-1 downto 0);
    s00_axi_arprot  : in std_logic_vector(2 downto 0);
    s00_axi_arvalid : in std_logic;
    s00_axi_arready : out std_logic;
    s00_axi_rdata : out std_logic_vector(C_S00_AXI_DATA_WIDTH-1 downto 0);
    s00_axi_rresp : out std_logic_vector(1 downto 0);
    s00_axi_rvalid  : out std_logic;
    s00_axi_rready  : in std_logic;
    
    -- HBICAP ICAP interface
    icap_csib_hbicap  : in std_logic;
    icap_rdwrb_hbicap : in std_logic;
    icap_i_hbicap     : in  std_logic_vector(31 downto 0);
    icap_o_hbicap     : out std_logic_vector(31 downto 0);

    -- HBICAP clock gating signal
    gate_icap_clk_hbicap : in std_logic;

    -- HBICAP arbiter interface
    req_hbicap  : in std_logic;
    gnt_hbicap : out std_logic;
    rel_hbicap  : out std_logic;

    -- Fine grain RE ICAP interface
    icap_csib_fgre  : in std_logic;
    icap_rdwrb_fgre : in std_logic;
    icap_i_fgre     : in  std_logic_vector(31 downto 0);
    icap_o_fgre     : out std_logic_vector(31 downto 0)    
  );
  --attribute MARK_DEBUG : string;
  --attribute mark_debug of icap_csib_hbicap: signal is "true";
  --attribute mark_debug of icap_rdwrb_hbicap: signal is "true";
  --attribute mark_debug of icap_i_hbicap: signal is "true";
  --attribute mark_debug of icap_o_hbicap: signal is "true";
  --attribute mark_debug of gate_icap_clk_hbicap: signal is "true";
  --attribute mark_debug of icap_csib_fgre: signal is "true";
  --attribute mark_debug of icap_rdwrb_fgre: signal is "true";
  --attribute mark_debug of icap_i_fgre: signal is "true";
  --attribute mark_debug of icap_o_fgre: signal is "true";
end mux_ICAP;

architecture Behavioral of mux_ICAP is
  
  signal icap_csib, icap_rdwrb, mux_selector : std_logic;
  signal icap_i, icap_o : std_logic_vector(31 downto 0);
  signal n_gate_icap_clk_hbicap, clock_gate, icap_clk : std_logic;

begin

  -- Comm
  AXI_INST : entity work.s_axi_ctrl
    generic map (
      C_S_AXI_DATA_WIDTH => C_S00_AXI_DATA_WIDTH,
      C_S_AXI_ADDR_WIDTH => C_S00_AXI_ADDR_WIDTH
    )
    port map (
      S_AXI_ACLK  => s00_axi_aclk,
      S_AXI_ARESETN => s00_axi_aresetn,
      S_AXI_AWADDR  => s00_axi_awaddr,
      S_AXI_AWPROT  => s00_axi_awprot,
      S_AXI_AWVALID => s00_axi_awvalid,
      S_AXI_AWREADY => s00_axi_awready,
      S_AXI_WDATA => s00_axi_wdata,
      S_AXI_WSTRB => s00_axi_wstrb,
      S_AXI_WVALID  => s00_axi_wvalid,
      S_AXI_WREADY  => s00_axi_wready,
      S_AXI_BRESP => s00_axi_bresp,
      S_AXI_BVALID  => s00_axi_bvalid,
      S_AXI_BREADY  => s00_axi_bready,
      S_AXI_ARADDR  => s00_axi_araddr,
      S_AXI_ARPROT  => s00_axi_arprot,
      S_AXI_ARVALID => s00_axi_arvalid,
      S_AXI_ARREADY => s00_axi_arready,
      S_AXI_RDATA => s00_axi_rdata,
      S_AXI_RRESP => s00_axi_rresp,
      S_AXI_RVALID  => s00_axi_rvalid,
      S_AXI_RREADY  => s00_axi_rready,
      mux_selector => mux_selector
    );

  -- Multiplexer
  icap_csib  <= icap_csib_hbicap when mux_selector = '0' else icap_csib_fgre;
  icap_rdwrb <= icap_rdwrb_hbicap when mux_selector = '0' else icap_rdwrb_fgre;
  icap_i     <= icap_i_hbicap when mux_selector = '0' else icap_i_fgre;
  icap_o_hbicap   <= icap_o when mux_selector = '0' else (others => '0');
  icap_o_fgre   <= icap_o when mux_selector = '1' else (others => '0');

  -- Clock gate for the ICAP clock
  BUFGCE_inst : BUFGCE
    port map (
      CE => clock_gate,
      I => s00_axi_aclk,
      O => icap_clk
    );

  n_gate_icap_clk_hbicap <= not gate_icap_clk_hbicap;
  clock_gate <= n_gate_icap_clk_hbicap when mux_selector = '0' else '1';

  -- ICAP
  ICAP_INST : ICAPE2
    generic map (
      ICAP_WIDTH => "X32"
    )
    port map (
      CLK   => icap_clk,
      CSIB  => icap_csib,
      I     => icap_i,
      O     => icap_o,
      RDWRB => icap_rdwrb
    );
  
  -- Arbiter
  gnt_hbicap <= '1';
  rel_hbicap <= '0';

end Behavioral;