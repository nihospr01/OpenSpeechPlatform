library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity sr8_rising_tb is
end entity;

architecture a of sr8_rising_tb is
	signal clk, reset, ld, sh, sin, sout: std_logic;
	signal pin, pout: std_logic_vector(7 downto 0);
	
	component sr8_falling is
		port(
			clk, reset: in std_logic;
			ld, sh: in std_logic;
			pin: in std_logic_vector(7 downto 0);
			pout: out std_logic_vector(7 downto 0);
			sin: in std_logic;
			sout: out std_logic
		);
	end component;
begin
	clk_gen: process is
		constant hr_clock_period : time := 10 ns;
	begin
		clk <= '0';
		loop
			wait for hr_clock_period/2;
			clk <= not clk;
		end loop;
	end process;
	reset <= '1', '0' after 30 ns;
	
	sh <= '0', '1' after 50 ns;
	sin <= '0', '1' after 85 ns, '0' after 115 ns;
	pin <= "10110101";
	ld <= '0', '1' after 200 ns;
	
	sr8_falling_e: sr8_falling port map(
		clk => clk, reset => reset, ld => ld, sh => sh,
		pin => pin, pout => pout, sin => sin, sout => sout
	);
end architecture;