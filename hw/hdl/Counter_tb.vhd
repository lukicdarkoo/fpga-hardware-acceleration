library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


architecture counterbhv of testbench is
	component Counter is
		port (
			Clk : in std_logic;
			nReset : in std_logic;
			Address : in std_logic_vector(2 downto 0);
			ChipSelect : in std_logic;
			Read : in std_logic;
			Write : in std_logic;
			ReadData : out std_logic_vector(31 downto 0);
			WriteData : in std_logic_vector(31 downto 0);
			IRQ : out std_logic
		);
	end component;

	signal Clk : std_logic;
	signal nReset : std_logic;
	signal Address : std_logic_vector(2 downto 0);
	signal ChipSelect : std_logic;
	signal Read : std_logic;
	signal Write : std_logic;
	signal ReadData : std_logic_vector (31 downto 0);
	signal WriteData : std_logic_vector (31 downto 0);

	constant CLK_PERIOD : time := 20 ns; --50 MHZ

begin
	UUT : Counter
	port map(
		Clk => Clk, 
		nReset => nReset, 
		Address => Address(2 downto 0), 
		ChipSelect => ChipSelect, 
		Read => Read, 
		Write => Write, 
		ReadData => ReadData, 
		WriteData => WriteData
	);

	-- Generate CLK signal
	clk_generation : process
	begin
		CLK <= '1';
		wait for CLK_PERIOD / 2;
		CLK <= '0';
		wait for CLK_PERIOD / 2;
	end process clk_generation;
	
	simulation : process begin
		wait for CLK_PERIOD;

		-- Reset
		nReset <= '0';
		wait for CLK_PERIOD;
		nReset <= '1';
		wait for CLK_PERIOD;
		
		
		ChipSelect <= '1';

		-- Start counter
		Address <= "001";
		Write <= '1';	
		
		wait for CLK_PERIOD;
		Address <= "010";
		Write <= '1';	
		wait for CLK_PERIOD;
		Write <= '0';	
	
		
		wait for 10*CLK_PERIOD;
		
		-- Stop counter
		Address <= "011";
		Write <= '1';	
		
		-- Read written data
		Write <= '0';
		Address <= "000";
		Read <= '1';
		
		
		wait for CLK_PERIOD;
		
	end process simulation;
end counterbhv;