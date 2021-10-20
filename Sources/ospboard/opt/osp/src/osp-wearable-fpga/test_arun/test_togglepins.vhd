library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity test_fifo_readclkpins is
	port(
		clock: in std_logic;
		--
		spi_clk, spi_cs: in std_logic;
		spi_miso: out std_logic;
		adc_clk, adc_pdwn: out std_logic;
		adc_data: in std_logic_vector(11 downto 0);
		interrupt: out std_logic
	);
end entity;

architecture a of test_fifo_readclkpins is
	signal interrupt_counter: unsigned(9 downto 0); 
	signal hacky_fix: unsigned(1 downto 0);
	
	signal counter_div12: unsigned(2 downto 0);
	signal clkdiv12: std_logic;
	
	signal fifo_readclk: std_logic;
	signal fifo_bit: unsigned(3 downto 0);
	
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
	
	signal rampcounter: unsigned(11 downto 0);
	
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
	

	adc_pdwn <= '0';
	
	process(clock) is begin
		if rising_edge(clock) then
			interrupt_counter <= interrupt_counter + 1;
			counter_div12 <= counter_div12 + 1;
			if counter_div12 = "101" then
				counter_div12 <= "000";
				clkdiv12 <= not clkdiv12;
			end if;
		end if;
	end process;
	
	adc_clk <= clkdiv12;
	
	process(clkdiv12) is begin
		if rising_edge(clkdiv12) then
			rampcounter <= rampcounter + 1;
			interrupt_counter <= interrupt_counter + 1;
		end if;
	end process;
	
	
	fifo_Data <= std_logic_vector(rampcounter); --adc_data;
	fifo_WrClock <= clkdiv12;
	fifo_RdClock <= fifo_readclk;
	fifo_WrEn <= '1';
	fifo_RdEn <= '1';
	fifo_Reset <= '0';
	fifo_RPReset <= '0';
	
	interrupt <= and interrupt_counter;
	
	
	process(spi_clk) is begin
		if rising_edge(spi_clk) then
			if spi_cs = '1' then
				hacky_fix <= "00";
				fifo_bit <= "0000";
			end if;
			spi_miso <= spi_data;
			spi_data <= fifo_Q(to_integer(unsigned(fifo_bit)));
			if spi_cs = '0' then
				if hacky_fix = "10" then
					if fifo_bit = "0101" then
						fifo_readclk <= not fifo_readclk;
					end if;
					if fifo_bit = "1011" then
						fifo_readclk <= not fifo_readclk;
						fifo_bit <= "0000";
					else
						fifo_bit <= fifo_bit + 1;
					end if;
				else
					hacky_fix <= hacky_fix + 1;
				end if;
			end if;
		end if;
	end process;
	
end architecture;