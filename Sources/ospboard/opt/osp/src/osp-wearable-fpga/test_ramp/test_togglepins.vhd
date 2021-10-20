library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity test_togglepins is
	port(
		clock: in std_logic;
		xtal_in: in std_logic;
		xtal_out: out std_logic;
		--
		spi_clk, spi_mosi, spi_cs: in std_logic;
		spi_miso: out std_logic;
		adc_clk, adc_clk_copy, adc_clk_copy2, adc_pdwn: out std_logic;
		adc_data: in std_logic_vector(11 downto 0);
		interrupt: out std_logic
	);
end entity;

architecture a of test_togglepins is
	signal counter: unsigned(14 downto 0); 
	signal counter2: unsigned(2 downto 0);
	signal counter3: unsigned(3 downto 0);
	signal counter4: unsigned(14 downto 0);
	signal rampcounter: unsigned(11 downto 0); 
	signal toggle: std_logic;
	signal spi_data: std_logic;
	signal fifo_Data: std_logic_vector(11 downto 0); 
	signal fifo_WrClock: std_logic; 
	signal fifo_RdClock:  std_logic; 
	signal fifo_WrEn: std_logic; 
	signal fifo_RdEn: std_logic; 
	signal fifo_Reset: std_logic; 
	signal fifo_RPReset: std_logic; 
	signal fifo_Q:  std_logic_vector(11 downto 0); 
	signal fifo_Empty:   std_logic; 
	signal fifo_Full:   std_logic; 
	signal fifo_AlmostEmpty:   std_logic; 
	signal fifo_AlmostFull:   std_logic;
	signal pll_clocki:   std_logic;
	signal pll_clocko:   std_logic;
	
component fifotest is
	port (
        Data: in  std_logic_vector(11 downto 0); 
        WrClock: in  std_logic; 
        RdClock: in  std_logic; 
        WrEn: in  std_logic; 
        RdEn: in  std_logic; 
        Reset: in  std_logic; 
        RPReset: in  std_logic; 
        Q: out  std_logic_vector(11 downto 0); 
        Empty: out  std_logic; 
        Full: out  std_logic; 
        AlmostEmpty: out  std_logic; 
        AlmostFull: out  std_logic);
end component;

component plltest is
    port (
        CLKI: in  std_logic; 
        CLKOP: out  std_logic);
end component;

	
begin
	fifo: fifotest
	port map(
		Data => fifo_Data,
        WrClock  => fifo_WrClock,
        RdClock  => fifo_RdClock,
        WrEn  => fifo_WrEn,
        RdEn  => fifo_RdEn, 
        Reset  => fifo_Reset, 
        RPReset  => fifo_RPReset, 
        Q  => fifo_Q,
        Empty  => fifo_Empty,
        Full  => fifo_Full,
        AlmostEmpty  => fifo_AlmostEmpty,
        AlmostFull  => fifo_AlmostFull
	);
	
	pll1: plltest
	port map(
		CLKI => pll_clocki,
		CLKOP => pll_clocko
	);
	
	pll_clocki <= clock;

	adc_pdwn <= '0';
	
	xtal_out <= not xtal_in;
	
	interrupt <= counter(14);
	
	adc_clk <= pll_clocko;
	adc_clk_copy <= counter2(2);
	adc_clk_copy2 <= pll_clocko;
	
	process(clock) is begin
		if rising_edge(clock) then
			counter <= counter + 1;
			counter2 <= counter2 + 1;
		end if;
	end process;
	
	fifo_Data <= adc_data;
	fifo_WrClock <= counter2(2);
	fifo_RdClock <= toggle;
	fifo_WrEn <= '1';
	fifo_RdEn <= '1';
	fifo_Reset <= '0';
	fifo_RPReset <= '0';
	
	--process(spi_cs) is begin
		--if falling_edge(spi_cs) then
			--counter3 <= "0000";
		--end if;
	--end process;
	
	process(spi_clk) is begin
		if rising_edge(spi_clk) then
			spi_miso <= spi_data;
			spi_data <= rampcounter(to_integer(unsigned(counter3)));--fifo_Q(to_integer(unsigned(counter3)));
			if counter3 = "0101" then
				toggle <= not toggle;
			end if;
			if counter3 = "1011" then
				toggle <= not toggle;
				counter3 <= "0000";
				rampcounter <= rampcounter + 1;
			else
				counter3 <= counter3 + 1;
			end if;
			if rampcounter = "111111111111" then
				rampcounter <= "000000000000";
			end if;
		end if;
	end process;
end architecture;