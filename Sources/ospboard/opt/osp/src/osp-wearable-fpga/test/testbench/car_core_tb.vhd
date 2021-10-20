library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity car_core_tb is
end entity;

architecture a of car_core_tb is
	component car_core is
		port(
			hr_clk, reset: in std_logic;
			--
			lvds_io: inout std_logic;
			--
			i2s_sclk: out std_logic;
			i2s_ws: out std_logic;
			i2s_dspk: in std_logic;
			i2s_dmic: out std_logic;
			--
			spi_sck: in std_logic;
			spi_mosi: in std_logic;
			spi_miso: out std_logic;
			spi_cs0: in std_logic;
			spi_cs1: in std_logic;
			--
			test_main_clk, test_seq_reset: out std_logic
		);
	end component;

	signal hr_clk, reset: std_logic;
	signal test_main_clk, test_seq_reset: std_logic;
	signal lvds_io: std_logic;
	signal i2s_sclk, i2s_ws, i2s_dmic, i2s_dspk: std_logic;
	signal spi_sck, spi_mosi, spi_miso, spi_cs0, spi_cs1: std_logic;
	
	constant hr_clock_period : time := 5.086263021 ns;
	type slarray is array (natural range <>) of std_logic;
begin
	spi_mosi <= '0';
	spi_sck <= '0';
	spi_cs0 <= '1';
	spi_cs1 <= '1';
	
	---------------------------------------------------------------------------
	
	hr_clk_gen: process is
	begin
		hr_clk <= '0';
		loop
			wait for hr_clock_period/2;
			hr_clk <= not hr_clk;
		end loop;
	end process;
	reset <= '1', '0' after 20 ns;
	
	---------------------------------------------------------------------------
	
	lvds_gen: process is
		constant lvds_wf: slarray := (
			'Z', 'Z',  'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',  'Z',  'Z', 'Z', 'Z', 'Z', 'Z',
			'0', '1',  '0', '0', '0', '0', '1', '1', '1', '1',  '0',  'Z', 'Z', 'Z', 'Z', 'Z',
			'Z', 'Z',  'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',  'Z',  'Z', 'Z', 'Z', 'Z', 'Z',
			'0', '1',  '1', '0', '1', '0', '1', '0', '0', '1',  '0',  'Z', 'Z', 'Z', 'Z', 'Z',
			'Z', 'Z',  'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',  'Z',  'Z', 'Z', 'Z', 'Z', 'Z',
			'0', '1',  '0', '1', '1', '0', '0', '0', '1', '0',  '1',  'Z', 'Z', 'Z', 'Z', 'Z',
			'Z', 'Z',  'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',  'Z',  'Z', 'Z', 'Z', 'Z', 'Z',
			'0', '1',  '1', '1', '0', '1', '0', '0', '0', '1',  '1',  'Z', 'Z', 'Z', 'Z', 'Z',
			'Z', 'Z',  'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',  'Z',  'Z', 'Z', 'Z', 'Z', 'Z',
			'0', '1',  '0', '0', '1', '1', '1', '0', '1', '1',  '0',  'Z', 'Z', 'Z', 'Z', 'Z',
			'Z', 'Z',  'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',  'Z',  'Z', 'Z', 'Z', 'Z', 'Z',
			'0', '1',  '0', '1', '0', '1', '1', '1', '1', '0',  '0',  'Z', 'Z', 'Z', 'Z', 'Z'
			);
	begin
		lvds_io <= 'Z';
		wait for 23 ns;
		for i in lvds_wf'range loop
			lvds_io <= lvds_wf(i);
			wait for 4*hr_clock_period;
		end loop;
		lvds_io <= 'Z';
		wait;
	end process;
	
	i2s_spk_gen: process is
		constant spkdata: slarray := (
			'1', '0', '1', '1', '0', '0', '1', '0',
			'0', '1', '0', '1', '1', '0', '0', '1',
			'1', '1', '1', '0', '0', '0', '1', '1',
			'0', '1', '1', '1', '0', '0', '0', '1',
			'1', '0', '1', '0', '0', '0', '1', '0',
			'1', '1', '0', '1', '1', '0', '0', '0'
			);
	begin
		for i in spkdata'range loop
			i2s_dspk <= spkdata(i);
			wait until falling_edge(i2s_sclk);
		end loop;
		i2s_dspk <= '0';
		wait;
	end process;
	
	---------------------------------------------------------------------------
	
	e_car_core: car_core port map (
		hr_clk => hr_clk, reset => reset, lvds_io => lvds_io,
		i2s_sclk => i2s_sclk, i2s_ws => i2s_ws, i2s_dmic => i2s_dmic, i2s_dspk => i2s_dspk,
		spi_sck => spi_sck, 
		spi_mosi => spi_mosi, 
		spi_cs0 => spi_cs0, 
		spi_cs1 => spi_cs1,
		spi_miso => spi_miso,
		test_main_clk => test_main_clk, test_seq_reset => test_seq_reset
	);
end architecture;