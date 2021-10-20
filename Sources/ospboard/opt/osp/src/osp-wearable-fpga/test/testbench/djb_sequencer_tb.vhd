library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity djb_sequencer_tb is
end entity;

architecture a of djb_sequencer_tb is
	signal hr_clk, main_clk, reset, seq_reset, seq_idle, lvds_io: std_logic;
	signal i2s_spkr_dat, i2s_spkr_sh, i2s_spkr_ld, i2s_mic_ld, i2s_mic_sh, i2s_mic_dat, i2s_lr_st: std_logic;
	signal spi_sck, spi_mosi, spi_miso, spi_cs0, spi_cs1: std_logic;
	signal test_lvdscounter: std_logic_vector(4 downto 0);

	component clock_sync is
		port(
			hr_clk, reset: in std_logic;
			seq_idle: in std_logic;
			lvds_i: in std_logic;
			main_clk: out std_logic;
			seq_reset: out std_logic;
			sync_early, sync_late: out std_logic
		);
	end component;
	
	component djb_sequencer is
		port(
			main_clk, reset: in std_logic;
			seq_reset: in std_logic;
			seq_idle: out std_logic;
			--
			lvds_io: inout std_logic;
			--
			i2s_spkr_dat, i2s_spkr_sh, i2s_spkr_ld: out std_logic;
			i2s_mic_ld, i2s_mic_sh: out std_logic;
			i2s_mic_dat: in std_logic;
			i2s_lr_st: out std_logic;
			--
			spi_sck, spi_mosi, spi_cs0, spi_cs1: out std_logic;
			spi_miso: in std_logic;
			--
			test_lvdscounter: out std_logic_vector(4 downto 0)
		);
	end component;
	
	constant hr_clock_period : time := 5.086263021 ns;
begin
	
	hr_clk_gen: process is
	begin
		hr_clk <= '0';
		loop
			wait for hr_clock_period/2;
			hr_clk <= not hr_clk;
		end loop;
	end process;
	reset <= '1', '0' after 20 ns;
	
	lvds_gen: process is
		type slarray is array (natural range <>) of std_logic;
		constant lvds_wf: slarray := (
			'0', '1',  '0', '0', '0', '0', '1', '1', '1', '1',  '0',  '1', '0', '1', '1', 'Z',
			'Z', 'Z',  'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',  'Z',  'Z', 'Z', 'Z', 'Z', 'Z');
	begin
		lvds_io <= '0';
		wait for 40 ns;
		for i in lvds_wf'range loop
			lvds_io <= lvds_wf(i);
			wait for 4*hr_clock_period;
		end loop;
		lvds_io <= '0';
		wait;
	end process;
	
	
	i2s_mic_dat <= '1';
	spi_miso <= '1';
	
	clock_sync_e: clock_sync port map(
		hr_clk => hr_clk,
		reset => reset,
		seq_idle => seq_idle,
		lvds_i => lvds_io,
		main_clk => main_clk,
		seq_reset => seq_reset,
		sync_early => open,
		sync_late => open
	);
	
	djb_sequencer_e: djb_sequencer port map(
		main_clk => main_clk, reset => reset,
		seq_reset => seq_reset, seq_idle => seq_idle, lvds_io => lvds_io,
		i2s_spkr_dat => i2s_spkr_dat, i2s_spkr_sh => i2s_spkr_sh, i2s_spkr_ld => i2s_spkr_ld,
		i2s_mic_ld => i2s_mic_ld, i2s_mic_sh => i2s_mic_sh, i2s_mic_dat => i2s_mic_dat,
		i2s_lr_st => i2s_lr_st,
		spi_sck => spi_sck, spi_mosi => spi_mosi, spi_miso => spi_miso, 
		spi_cs0 => spi_cs0, spi_cs1 => spi_cs1,
		test_lvdscounter => test_lvdscounter
	);
end architecture;
