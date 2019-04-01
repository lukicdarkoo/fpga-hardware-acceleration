-- Formatted using: https://chan.mit.edu/vhdl-formatter

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity Swap is
	port (
		dataa : in std_logic_vector(31 downto 0);
		-- datab : in std_logic_vector(31 downto 0);
		result : out std_logic_vector(31 downto 0)
	);
end Swap;

architecture comp of Swap is

begin

 	result(7 downto 0) <= dataa(31 downto 24);
	result(31 downto 24) <= dataa(7 downto 0);
	
	swap_bits: for i in 8 to 23 generate
      result(i) <= dataa(31 - i);
	end generate swap_bits;
	
end comp;
