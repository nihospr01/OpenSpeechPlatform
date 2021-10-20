library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity all_tb is
end entity;

architecture a of all_tb is
	component OSP_Carrier_Basic is
		port(
			db_clk: in std_logic;
			db_reset: in std_logic;
			db_switches: in std_logic_vector(3 downto 0);
			db_leds: out std_logic_vector(7 downto 0);
			--
			l_lvds_io: inout std_logic;
			r_lvds_io: inout std_logic;
			codec_clk: in std_logic;
			--
			i2s1_sck: out std_logic;
			i2s1_ws: out std_logic;
			i2s1_d0: in std_logic;
			i2s1_d1: in std_logic;
			i2s2_sck: out std_logic;
			i2s2_ws: out std_logic;
			i2s2_d0: out std_logic;
			i2s2_d1: out std_logic;
			--
			spi1_clk: in std_logic;
			spi1_mosi: in std_logic;
			spi1_miso: out std_logic;
			spi1_cs0: in std_logic;
			spi1_cs2: in std_logic;
			spi3_clk: in std_logic;
			spi3_mosi: in std_logic;
			spi3_miso: out std_logic;
			spi3_cs0: in std_logic;
			spi3_cs3: in std_logic
		);
	end component;
	component OWF_test is
		port(
			db_clk: in std_logic;
			db_reset: in std_logic;
			db_switches: in std_logic_vector(3 downto 0);
			db_leds: out std_logic_vector(7 downto 0);
			--
			lvds_io: inout std_logic;
			codec_clk: in std_logic;
			--
			i2s_sck: out std_logic;
			i2s_ws: out std_logic;
			i2s_dmic: in std_logic;
			i2s_dspk: out std_logic;
			--
			spi_clk: out std_logic;
			spi_mosi: out std_logic;
			spi_miso: in std_logic;
			spi_cs0: out std_logic;
			spi_cs1: out std_logic
		);
	end component;
	
	signal djb_hr_clk, djb_reset, djb_main_clk: std_logic;
	signal car_hr_clk, car_reset, car_main_clk: std_logic;
	signal lvds_io: std_logic;
	signal djb_i2s_sclk, djb_i2s_ws, djb_i2s_dspk, djb_i2s_dmic: std_logic;
	signal car_i2s_sclk, car_i2s_ws, car_i2s_dspk, car_i2s_dmic: std_logic;
	signal djb_spi_sck, djb_spi_mosi, djb_spi_miso, djb_spi_cs0, djb_spi_cs1: std_logic;
	signal car_spi_sck, car_spi_mosi, car_spi_miso, car_spi_cs0, car_spi_cs1: std_logic;
	
	constant hr_clock_period : time := 5.086263021 ns;
	type slarray is array (natural range <>) of std_logic;
begin

	---------------------------------------------------------------------------
	
	djb_hr_clk_gen: process is
	begin
		djb_hr_clk <= '0';
		loop
			wait for hr_clock_period/2;
			djb_hr_clk <= not djb_hr_clk;
		end loop;
	end process;
	djb_reset <= '1', '0' after 20 ns;
	
	car_hr_clk_gen: process is
	begin
		car_hr_clk <= '0';
		loop
			wait for hr_clock_period/2;
			car_hr_clk <= not car_hr_clk;
		end loop;
	end process;
	car_reset <= '1', '0' after 3.88 us;
	
	---------------------------------------------------------------------------
	
	djb_i2s_mic_gen: process is
		constant micdata: slarray := (
			'1', '1', '1', '1', '0', '1', '1', '0', 
			'0', '1', '1', '1', '0', '0', '0', '0', 
			'1', '1', '0', '0', '0', '1', '0', '0', 
			'1', '0', '1', '0', '0', '1', '1', '1', 
			'0', '0', '1', '1', '1', '0', '1', '1', 
			'0', '1', '1', '0', '1', '1', '0', '0', 
			'1', '0', '1', '1', '0', '0', '1', '0', 
			'1', '1', '0', '1', '1', '1', '1', '0', 
			'0', '1', '1', '1', '0', '0', '1', '1', 
			'0', '0', '1', '1', '1', '0', '0', '1', 
			'1', '1', '1', '0', '0', '0', '1', '0', 
			'1', '0', '1', '0', '1', '1', '1', '1', 
			'0', '0', '0', '0', '0', '0', '1', '0', 
			'1', '1', '0', '0', '0', '0', '1', '0', 
			'1', '1', '0', '1', '1', '1', '1', '1', 
			'0', '0', '0', '1', '1', '1', '0', '1', 
			'1', '0', '0', '1', '1', '1', '0', '0'
			);
	begin
		loop
			for i in micdata'range loop
				djb_i2s_dmic <= micdata(i);
				wait until falling_edge(djb_i2s_sclk);
			end loop;
		end loop;
	end process;
	
	car_i2s_spk_gen: process is
		constant spkdata: slarray := (
			'0', '1', '1', '0', '1', '1', '0', '1', 
			'0', '0', '1', '0', '1', '0', '1', '0', 
			'0', '0', '1', '0', '1', '0', '1', '0', 
			'0', '1', '1', '1', '1', '0', '1', '1', 
			'0', '1', '1', '1', '0', '0', '0', '1', 
			'1', '1', '0', '1', '0', '0', '1', '0', 
			'0', '1', '0', '0', '0', '0', '0', '0', 
			'0', '1', '1', '1', '0', '1', '1', '1', 
			'1', '0', '0', '0', '0', '1', '0', '1', 
			'1', '1', '1', '1', '1', '0', '0', '1', 
			'1', '0', '1', '0', '1', '1', '0', '0', 
			'0', '1', '0', '0', '1', '0', '0', '0', 
			'0', '0', '0', '1', '0', '1', '1', '1', 
			'1', '1', '0', '1', '1', '0', '1', '0', 
			'0', '0', '0', '0', '0', '1', '0', '0', 
			'0', '1', '1', '1', '0', '1', '0', '1', 
			'1', '0', '1', '0', '0', '1', '0', '0'
			);
	begin
		loop
			for i in spkdata'range loop
				car_i2s_dspk <= spkdata(i);
				wait until falling_edge(car_i2s_sclk);
			end loop;
		end loop;
	end process;
	
	---------------------------------------------------------------------------
	
	car_spi_sck_gen: process is begin
		car_spi_sck <= '0';
		loop
			wait for 5 us; -- SPI clock period is 10 us -- 100kHz
			car_spi_sck <= not car_spi_sck;
		end loop;
	end process;
	
	djb_spi_miso <= djb_spi_mosi;
	car_spi_mosi <= not car_spi_miso;
	car_spi_cs0 <= '1';
	car_spi_cs1 <= '1';
	
	---------------------------------------------------------------------------
	
	e_OSP_Carrier_Basic: OSP_Carrier_Basic port map (
		db_clk => '0',
		db_reset => car_reset,
		db_switches => "0000",
		db_leds => open,
		--
		l_lvds_io => lvds_io,
		r_lvds_io => open,
		codec_clk => 
		--
		i2s1_sck: out std_logic;
		i2s1_ws: out std_logic;
		i2s1_d0: in std_logic;
		i2s1_d1: in std_logic;
		i2s2_sck: out std_logic;
		i2s2_ws: out std_logic;
		i2s2_d0: out std_logic;
		i2s2_d1: out std_logic;
		--
		spi1_clk: in std_logic;
		spi1_mosi: in std_logic;
		spi1_miso: out std_logic;
		spi1_cs0: in std_logic;
		spi1_cs2: in std_logic;
		spi3_clk: in std_logic;
		spi3_mosi: in std_logic;
		spi3_miso: out std_logic;
		spi3_cs0: in std_logic;
		spi3_cs3: in std_logic
	);
	
	
end architecture;