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
    
    -- ICAP interface 0
    icap_csib_0  : in std_logic;
    icap_rdwrb_0 : in std_logic;
    icap_i_0     : in  std_logic_vector(31 downto 0);
    icap_o_0     : out std_logic_vector(31 downto 0);

    -- ICAP interface 0
    icap_csib_1  : in std_logic;
    icap_rdwrb_1 : in std_logic;
    icap_i_1     : in  std_logic_vector(31 downto 0);
    icap_o_1     : out std_logic_vector(31 downto 0);
    
    -- ICAP arbiter interface
    req  : in std_logic;
    gnt : out std_logic;
    rel  : out std_logic
  );
end mux_ICAP;

architecture Behavioral of mux_ICAP is

  signal icap_csib, icap_rdwrb, mux_selector, req_reg : std_logic;
  signal icap_i, icap_o : std_logic_vector(31 downto 0);

begin

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
  icap_csib  <= icap_csib_0 when mux_selector = '0' else icap_csib_1;
  icap_rdwrb <= icap_rdwrb_0 when mux_selector = '0' else icap_rdwrb_1;
  icap_i     <= icap_i_0 when mux_selector = '0' else icap_i_1;
  icap_o_0   <= icap_o when mux_selector = '0' else (others => '0');
  icap_o_1   <= icap_o when mux_selector = '1' else (others => '0');


  ICAP_INST : ICAPE2
    generic map (
      ICAP_WIDTH => "X32"
    )
    port map (
      CLK   => s00_axi_aclk,
      CSIB  => icap_csib,
      I     => icap_i,
      O     => icap_o,
      RDWRB => icap_rdwrb
    );

  process (s00_axi_aclk)
  begin
    if rising_edge(s00_axi_aclk) then 
      if s00_axi_aresetn = '0' then
        req_reg <= '0';
      else
        req_reg <= req;
      end if;
    end if;
  end process; 
    
  gnt <= req_reg;
  rel <= '0';

end Behavioral;