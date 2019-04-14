library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity SwapAccelerator is
	port (
		clk : in std_logic;
		nReset : in std_logic;

		avs_Add : in std_logic_vector(1 downto 0);
		avs_CS : in std_logic;
		avs_Wr : in std_logic;
		avs_Rd : in std_logic;
		avs_WrData : in std_logic_vector(31 downto 0);
		avs_RdData : out std_logic_vector(31 downto 0);

		avm_Add : out std_logic_vector(31 downto 0);
		avm_BE : out std_logic_vector(3 downto 0);
		avm_Wr : out std_logic;
		avm_Rd : out std_logic;
		avm_WrData : out std_logic_vector(31 downto 0);
		avm_RdData : in std_logic_vector(31 downto 0);
		avm_WaitRequest : in std_logic
	);
end entity SwapAccelerator;

architecture comp of SwapAccelerator is
	signal RegAddStart : std_logic_vector(31 downto 0);
	signal RegLgt : std_logic_vector(15 downto 0); 
	signal DataRd : std_logic_vector(31 downto 0); 
	signal Start, Finish : std_logic; 
	signal CntAdd : unsigned(31 downto 0); 
	signal CntLgt : unsigned (15 downto 0); 
	signal ValMin, ValMax : signed(31 downto 0); 
	signal ValXOR : std_logic_vector (31 downto 0); 
	type SM is(Idle, LdParam, RdAcc, WaitRd, WrSwap, WrAcc);
	signal StateM : SM;
begin
	AvalonSlaveWr : 
	process (clk, nReset)
	begin
		if nReset = '0' then
			RegAddStart <= (others => '0');
			Start <= '0';
		elsif rising_edge(clk) then
			Start <= '0';
			if avs_CS = '1' and avs_Wr = '1' then
				case avs_Add is
					when "00" => RegAddStart <= avs_WrData;
					when "01" => RegLgt <= avs_WrData(15 downto 0);
					when "10" => Start <= avs_WrData(0);
					when others => null;
				end case;
			end if;
		end if;
	end process AvalonSlaveWr;

	AvalonSlaveRd : 
	process (clk)
	begin
		if rising_edge(clk) then
			if avs_CS = '1' and avs_Rd = '1' then
				avs_RdData <= (others => '0'); 
				case avs_Add is
					when "00" => avs_RdData <= RegAddStart;
					when "01" => avs_RdData(15 downto 0) <= RegLgt;
					when "10" => avs_RdData(0) <= Start;
					when "11" => avs_RdData(0) <= Finish;
					when others => null;
				end case;
			end if;
		end if;
	end process AvalonSlaveRd;
	
	AvalonMaster : 
	process (clk, nReset)
	begin
		if nReset = '0' then
			Finish <= '0';
			CntAdd <= (others => '0');
			CntLgt <= (others => '0');
			StateM <= Idle;
		elsif rising_edge(clk) then
			case StateM is
				when Idle => 
					avm_Add <= (others => '0');
					avm_BE <= "0000";
					avm_Wr <= '0';
					avm_Rd <= '0';
					if Start = '1' then
						StateM <= LdParam; 
						Finish <= '0';
					end if;
				when LdParam => 
					CntAdd <= unsigned(RegAddStart);
					CntLgt <= unsigned(RegLgt);
					StateM <= RdAcc;
				when RdAcc => 
					avm_Add <= std_logic_vector(CntAdd);
					avm_BE <= "1111";
					avm_Rd <= '1';
					StateM <= WaitRd;
				when WaitRd => 
					if avm_WaitRequest = '0' then
						DataRd <= avm_RdData; 
						avm_Rd <= '0';
						StateM <= WrAcc;
					end if;
				when WrAcc => 
						avm_Add <= std_logic_vector(CntAdd);
						avm_BE <= "1111";
						avm_Wr <= '1';
						StateM <= WrSwap;
				when WrSwap =>
					if avm_WaitRequest = '0' then
						avm_Wr <= '0';
					
						-- Write swapped bits
						avm_WrData(7 downto 0) <= DataRd(31 downto 24);
						avm_WrData(31 downto 24) <= DataRd(7 downto 0);
						
						swap_bits_acc: for i in 8 to 23 loop
							avm_WrData(i) <= DataRd(31 - i);
						end loop swap_bits_acc;
						
						-- Next element
						CntAdd <= CntAdd + 4; 
						if CntLgt > 1 then 
							StateM <= RdAcc; 
							CntLgt <= CntLgt - 1; 
						else
							StateM <= Idle;
							Finish <= '1';
						end if;
					end if;
				when others => null;
			end case;
		end if;
	end process AvalonMaster;

end comp;
