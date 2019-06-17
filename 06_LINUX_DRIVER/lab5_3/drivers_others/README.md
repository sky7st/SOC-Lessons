#README

+ driver_and_others

	課本第五章實驗三的driver source code

	+ test資料夾

		請將test資料夾放到/linux-xlnx/drivers/下 (課本5-40)

	+ Makefile

		請先備份/linux-xlnx/drivers/Makefile

		並將此Makefile放到/linux-xlnx/drivers/下 (課本5-40)

	+ zynq-7000.dtsi & zynq-zed.dts

		請按照第四章生成devicetree方法，將此2檔案覆蓋原本的(記得備份)

		生成新的devicetree

+ hardware

	課本第五章實驗三的hdl設計

	本實驗在block design中只需要使用zynq7 processing system

	並在 MIO Configuration中按照 PSConfig.JPG 設定後

	依照前面章節，生成對應的硬體(bit檔, fsbl => boot.bin)

	![PSConfig.JPG](https://github.com/sky7st/SOC-Lessons/blob/master/06_LINUX_DRIVER/lab5_3/hardware/PSConfig.JPG)

	+ lab5_3.xdc

		本實驗硬體設計使用的XDC file

+ result 

	本次實驗在linux開機後產生的putty.log檔

	可以在putty.log檔案[173:181]行中看到

	驅動程式確實在開機過程載入devicetree時被載入