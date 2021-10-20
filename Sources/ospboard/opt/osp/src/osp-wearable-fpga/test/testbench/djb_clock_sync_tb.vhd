library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity djb_clock_sync_tb is
end entity;

architecture a of djb_clock_sync_tb is
	signal hr_clk, reset, seq_idle, lvds_i, main_clk, seq_reset, sync_early, sync_late: std_logic;
	
	component djb_clock_sync is
		port(
			hr_clk, reset: in std_logic;
			seq_idle: in std_logic;
			lvds_i: in std_logic;
			main_clk: out std_logic;
			seq_reset: out std_logic;
			sync_early, sync_late: out std_logic
		);
	end component;
begin
	hr_clk_gen: process is
		constant hr_clock_period : time := 5.086263021 ns;
	begin
		hr_clk <= '0';
		loop
			wait for hr_clock_period/2;
			hr_clk <= not hr_clk;
		end loop;
	end process;
	reset <= '1', '0' after 20 ns;
	
	seq_idle <= '0', '1' after 40 ns, '0' after 80 ns;
	lvds_i <= '1', '0' after 41 ns, '1' after 50 ns, '0' after 60 ns;
	
	djb_clock_sync_e: djb_clock_sync port map(
		hr_clk => hr_clk,
		reset => reset,
		seq_idle => seq_idle,
		lvds_i => lvds_i,
		main_clk => main_clk,
		seq_reset => seq_reset,
		sync_early => sync_early,
		sync_late => sync_late
	);
end architecture;
