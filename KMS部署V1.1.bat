@echo off
%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
cd /d "%~dp0"
title KMS服务器一键部署脚本V1.1
color 2

:start
echo -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
echo 本脚本由Leslie Alexander编写，基于Github开源项目vlmcsd，旨在部署KMS服务器
echo 注意，KMS服务器不能激活KMS服务器本身
echo 本脚本默认使用目录下的vlmcsd.exe（该程序为64位），如果你运行在32位的Windows上，请手动删除原本的vlmcsd.exe，然后把vlmcsd_x32.exe重命名为vlmcsd.exe
echo 仅供学习和研究使用，一切使用该脚本的盗版行为与作者无关
echo Telegram:@LeslieAlexander E-mail:banspam@vtqpy.onmicrosoft.com
echo 继续则默认您同意以上说明
echo -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

echo 请输入要执行的操作：
echo [install]开始部署服务
echo [uninstall]移除服务
echo [stop]停止服务
echo [start]启动服务
echo [key]查看各版本Windows及Office密钥
echo [readme]关于作者
set /p choice=请输入选择：
if /i "%choice%"=="install" goto install
if /i "%choice%"=="uninstall" goto uninstall
if /i "%choice%"=="stop" goto stop
if /i "%choice%"=="start" goto setup
if /i "%choice%"=="key" goto key
if /i "%choice%"=="readme" goto readme

:install
echo 即将开始部署，请键入日志文件存放地址：
set /p URI=请输入地址：
echo 正在部署...
vlmcsd.exe -s -l %URI%\KMS.log
echo 正在启动服务...
sc query state= inactive | findstr /c:"Key Management Server"
net start "Key Management Server"
echo 正在添加防火墙入站规则...
netsh advfirewall firewall add rule name=KMS1688 dir=in action=allow protocol=TCP localport=1688
pause
cls&goto start

:uninstall
echo 您确认要移除服务吗？
pause
echo 正在移除服务...
sc query state= inactive | findstr /c:"Key Management Server"
net stop "Key Management Server"
sc delete vlmcsd
echo 正在移除防火墙规则...
netsh advfirewall firewall delete rule name=KMS1688
echo 移除成功！请手动选择是否删除日志文件
pause
cls&goto start

:stop
sc query state= inactive | findstr /c:"Key Management Server"
net stop "Key Management Server"
pause
cls&goto start

:start
sc query state= inactive | findstr /c:"Key Management Server"
net start "Key Management Server"
pause
cls&goto start

:key
echo Windows 10 Pro 	W269N-WFGWX-YVC9B-4J6C9-T83GX
echo Windows 10 Pro N 	MH37W-N47XK-V7XM9-C7227-GCQG9
echo Windows 10 Pro Workstations 	NRG8B-VKK3Q-CXVCJ-9G2XF-6Q84J
echo Windows 10 Pro Workstations N 	9FNHH-K3HBT-3W4TD-6383H-6XYWF
echo Windows 10 Pro Education 	6TP4R-GNPTD-KYYHQ-7B7DP-J447Y
echo Windows 10 Pro Education N 	YVWGF-BXNMC-HTQYQ-CPQ99-66QFC
echo Windows 10 Education 	NW6C2-QMPVW-D7KKK-3GKT6-VCFB2
echo Windows 10 Education N 	2WH4N-8QGBV-H22JP-CT43Q-MDWWJ
echo Windows 10 Enterprise 	NPPR9-FWDCX-D2C8J-H872K-2YT43
echo Windows 10 Enterprise N 	DPH2V-TTNVB-4X9Q3-TJR4H-KHJW4
echo Windows 10 Enterprise G 	YYVX9-NTFWV-6MDM3-9PT4T-4M68B
echo Windows 10 Enterprise G N 	44RPN-FTY23-9VTTB-MP9BX-T84FV
echo Windows 10 Enterprise LTSC 2019 	M7XTQ-FN8P6-TTKYV-9D4CC-J462D
echo Windows 10 Enterprise N LTSC 2019 	92NFX-8DJQP-P6BBQ-THF9C-7CG2H
echo Windows 10 Enterprise LTSB 2016 	DCPHK-NFMTC-H88MJ-PFHPY-QJ4BJ
echo Windows 10 Enterprise N LTSB 2016 	QFFDN-GRT3P-VKWWX-X7T3R-8B639
echo Windows 10 Enterprise LTSB 2015 	WNMTR-4C88C-JK8YV-HQ7T2-76DF9
echo Windows 10 Enterprise N LTSB 2015 	2F77B-TNFGY-69QQF-B8YKP-D69TJ
echo Windows 8.1 Pro 	GCRJD-8NW9H-F2CDX-CCM8D-9D6T9
echo Windows 8.1 Pro N 	HMCNV-VVBFX-7HMBH-CTY9B-B4FXY
echo Windows 8.1 Enterprise 	MHF9N-XY6XB-WVXMC-BTDCT-MKKG7
echo Windows 8.1 Enterprise N 	TT4HM-HN7YT-62K67-RGRQJ-JFFXW
echo Windows 8 Pro 	NG4HW-VH26C-733KW-K6F98-J8CK4
echo Windows 8 Pro N 	XCVCF-2NXM9-723PB-MHCB7-2RYQQ
echo Windows 8 Enterprise 	32JNW-9KQ84-P47T8-D8GGY-CWCK7
echo Windows 8 Enterprise N 	JMNMF-RHW7P-DMY6X-RF3DR-X2BQT
echo Windows 7 Professional 	FJ82H-XT6CR-J8D7P-XQJJ2-GPDD4
echo Windows 7 Professional N 	MRPKT-YTG23-K7D7T-X2JMM-QY7MG
echo Windows 7 Professional E 	W82YF-2Q76Y-63HXB-FGJG9-GF7QX
echo Windows 7 Enterprise 	33PXH-7Y6KF-2VJC9-XBBR8-HVTHH
echo Windows 7 Enterprise N 	YDRBP-3D83W-TY26F-D46B2-XCKRJ
echo Windows 7 Enterprise E 	C29WB-22CC8-VJ326-GHFJW-H9DH4
echo Windows Server 2019 Datacenter 	WMDGN-G9PQG-XVVXX-R3X43-63DFG
echo Windows Server 2019 Standard 	N69G4-B89J2-4G8F4-WWYCC-J464C
echo Windows Server 2019 Essentials 	WVDHN-86M7X-466P6-VHXV7-YY726
echo Windows Server 2016 Datacenter 	CB7KF-BWN84-R7R2Y-793K2-8XDDG
echo Windows Server 2016 Standard 	WC2BQ-8NRM3-FDDYY-2BFGV-KHKQY
echo Windows Server 2016 Essentials 	JCKRF-N37P4-C2D82-9YXRT-4M63B
echo Windows Server 2012 R2 Datacenter 	W3GGN-FT8W3-Y4M27-J84CP-Q3VJ9
echo Windows Server 2012 R2 Standard 	D2N9P-3P6X9-2R39C-7RTCD-MDVJX
echo Windows Server 2012 R2 Essentials 	KNC87-3J2TX-XB4WP-VCPJV-M4FWM
echo Windows Server 2012 	BN3D2-R7TKB-3YPBD-8DRP2-27GG4
echo Windows Server 2012 N 	8N2M2-HWPGY-7PGT9-HGDD8-GVGGY
echo Windows Server 2012 Single Language 	2WN2H-YGCQR-KFX6K-CD6TF-84YXQ
echo Windows Server 2012 Country Specific 	4K36P-JN4VD-GDC6V-KDT89-DYFKP
echo Windows Server 2012 Standard 	XC9B7-NBPP2-83J2H-RHMBY-92BT4
echo Windows Server 2012 MultiPoint Standard 	HM7DN-YVMH3-46JC3-XYTG7-CYQJJ
echo Windows Server 2012 MultiPoint Premium 	XNH6W-2V9GX-RGJ4K-Y8X6F-QGJ2G
echo Windows Server 2012 Datacenter 	48HP8-DN98B-MYWDG-T2DCC-8W83P
echo Windows Server 2008 R2 Web 	6TPJF-RBVHG-WBW2R-86QPH-6RTM4
echo Windows Server 2008 R2 HPC edition 	TT8MH-CG224-D3D7Q-498W2-9QCTX
echo Windows Server 2008 R2 Standard 	YC6KT-GKW9T-YTKYR-T4X34-R7VHC
echo Windows Server 2008 R2 Enterprise 	489J6-VHDMP-X63PK-3K798-CPX3Y
echo Windows Server 2008 R2 Datacenter 	74YFP-3QFB3-KQT8W-PMXWJ-7M648
echo Windows Server 2008 R2 for Itanium-based Systems 	GT63C-RJFQ3-4GMB6-BRFB9-CB83V
echo Windows Web Server 2008 	WYR28-R7TFJ-3X2YQ-YCY4H-M249D
echo Windows Server 2008 Standard 	TM24T-X9RMF-VWXK6-X8JC9-BFGM2
echo Windows Server 2008 Standard without Hyper-V 	W7VD6-7JFBR-RX26B-YKQ3Y-6FFFJ
echo Windows Server 2008 Enterprise 	YQGMW-MPWTJ-34KDK-48M3W-X4Q6V
echo Windows Server 2008 Enterprise without Hyper-V 	39BXF-X8Q23-P2WWT-38T2F-G3FPG
echo Windows Server 2008 HPC 	RCTX3-KWVHP-BR6TB-RB6DM-6X7HP
echo Windows Server 2008 Datacenter 	7M67G-PC374-GR742-YH8V4-TCBY3
echo Windows Server 2008 Datacenter without Hyper-V 	22XQ2-VRXRG-P8D42-K34TD-G3QQC
echo Windows Server 2008 for Itanium-Based Systems 	4DWFP-JF3DJ-B7DTH-78FJB-PDRHK
echo Windows Server Datacenter, version 1809/1903 	6NMRW-2C8FM-D24W7-TQWMY-CWH2D
echo Windows Server Standard, version 1809/1903 	N2KJX-J94YW-TQVFB-DG9YT-724CC
echo Windows Server Datacenter, version 1803 	2HXDN-KRXHB-GPYC7-YCKFJ-7FVDG
echo Windows Server Standard, version 1803 	PTXN8-JFHJM-4WC78-MPCBR-9W4KR
echo Windows Server Datacenter, version 1709 	6Y6KB-N82V8-D8CQV-23MJW-BWTG6
echo Windows Server Standard, version 1709 	DPCNP-XQFKJ-BJF7R-FRC8D-GF6G4

echo Office Professional Plus 2021 	FXYTK-NJJ8C-GB6DW-3DYQT-6F7TH
echo Office Standard 2021 	KDX7X-BNVR8-TXXGX-4Q7Y8-78VT3
echo Project Professional 2021 	FTNWT-C6WBT-8HMGF-K9PRX-QV9H8
echo Project Standard 2021 	J2JDC-NJCYY-9RGQ4-YXWMH-T3D4T
echo Visio Professional 2021 	KNH8D-FGHT4-T8RK3-CTDYJ-K2HT4
echo Visio Standard 2021 	MJVNY-BYWPY-CWV6J-2RKRT-4M8QG
echo Access 2021 	WM8YG-YNGDD-4JHDC-PG3F4-FC4T4
echo Excel 2021 	NWG3X-87C9K-TC7YY-BC2G7-G6RVC
echo Outlook 2021 	C9FM6-3N72F-HFJXB-TM3V9-T86R9
echo PowerPoint 2021 	TY7XF-NFRBR-KJ44C-G83KF-GX27K
echo Publisher 2021 	2MW9D-N4BXM-9VBPG-Q7W6M-KFBGQ
echo Skype for Business 2021 	HWCXN-K3WBT-WJBKY-R8BD9-XK29P
echo Word 2019 	TN8H9-M34D3-Y64V9-TR72V-X79KV
echo Office Professional Plus 2019 	NMMKJ-6RK4F-KMJVX-8D9MJ-6MWKP
echo Office Standard 2019 	6NWWJ-YQWMR-QKGCB-6TMB3-9D9HK
echo Project Professional 2019 	B4NPR-3FKK7-T2MBV-FRQ4W-PKD2B
echo Project Standard 2019 	C4F7P-NCP8C-6CQPT-MQHV9-JXD2M
echo Visio Professional 2019 	9BGNQ-K37YR-RQHF2-38RQ3-7VCBB
echo Visio Standard 2019 	7TQNQ-K3YQQ-3PFH7-CCPPM-X4VQ2
echo Access 2019 	9N9PT-27V4Y-VJ2PD-YXFMF-YTFQT
echo Excel 2019 	TMJWT-YYNMB-3BKTF-644FC-RVXBD
echo Outlook 2019 	7HD7K-N4PVK-BHBCQ-YWQRW-XW4VK
echo PowerPoint 2019 	RRNCX-C64HY-W2MM7-MCH9G-TJHMQ
echo Publisher 2019 	G2KWX-3NW6P-PY93R-JXK2T-C9Y9V
echo Skype for Business 2019 	NCJ33-JHBBY-HTK98-MYCV8-HMKHJ
echo Word 2019 	PBX3G-NWMT6-Q7XBW-PYJGG-WXD33
echo Office Professional Plus 2016 	XQNVK-8JYDB-WJ9W3-YJ8YR-WFG99
echo Office Mondo 2016 	HFTND-W9MK4-8B7MJ-B6C4G-XQBR2
echo Office Standard 2016 	JNRGM-WHDWX-FJJG3-K47QV-DRTFM
echo Project Professional 2016 	YG9NW-3K39V-2T3HJ-93F3Q-G83KT
echo Project Standard 2016 	GNFHQ-F6YQM-KQDGJ-327XX-KQBVC
echo Visio Professional 2016 	PD3PC-RHNGV-FXJ29-8JK7D-RJRJK
echo Visio Standard 2016 	7WHWN-4T7MP-G96JF-G33KR-W8GF4
echo Access 2016 	GNH9Y-D2J4T-FJHGG-QRVH7-QPFDW
echo Excel 2016 	9C2PK-NWTVB-JMPW8-BFT28-7FTBF
echo OneNote 2016 	DR92N-9HTF2-97XKM-XW2WJ-XW3J6
echo Outlook 2016 	R69KK-NTPKF-7M3Q4-QYBHW-6MT9B
echo PowerPoint 2016 	J7MQP-HNJ4Y-WJ7YM-PFYGF-BY6C6
echo Publisher 2016 	F47MM-N3XJP-TQXJ9-BP99D-8K837
echo Skype for Business 2016 	869NQ-FJ69K-466HW-QYCP2-DDBV6
echo Word 2016 	WXY84-JN2Q9-RBCCQ-3Q3J3-3PFJ6
echo Office Professional Plus 2013 	YC7DK-G2NP3-2QQC3-J6H88-GVGXT
echo Office Mondo 2013 	42QTK-RN8M7-J3C4G-BBGYM-88CYV
echo Office Standard 2013 	KBKQT-2NMXY-JJWGP-M62JB-92CD4
echo Project Professional 2013 	FN8TT-7WMH6-2D4X9-M337T-2342K
echo Project Standard 2013 	6NTH3-CW976-3G3Y2-JK3TX-8QHTT
echo Visio Professional 2013 	C2FG9-N6J68-H8BTJ-BW3QX-RM3B3
echo Visio Standard 2013 	J484Y-4NKBF-W2HMG-DBMJC-PGWR7
echo Access 2013 	NG2JY-H4JBT-HQXYP-78QH9-4JM2D
echo Excel 2013 	VGPNG-Y7HQW-9RHP7-TKPV3-BG7GB
echo InfoPath 2013 	DKT8B-N7VXH-D963P-Q4PHY-F8894
echo Lync 2013 	2MG3G-3BNTT-3MFW9-KDQW3-TCK7R
echo OneNote 2013 	TGN6P-8MMBC-37P2F-XHXXK-P34VW
echo Outlook 2013 	QPN8Q-BJBTJ-334K3-93TGY-2PMBT
echo PowerPoint 2013 	4NT99-8RJFH-Q2VDH-KYG2C-4RD4F
echo Publisher 2013 	PN2WF-29XG2-T9HJ7-JQPJR-FCXK4
echo Word 2013 	6Q7VD-NX8JD-WJ2VH-88V73-4GBJ7
echo Office Professional Plus 2010 	VYBBJ-TRJPB-QFQRF-QFT4D-H3GVB
echo Office Mondo 2010 	YBJTT-JG6MD-V9Q7P-DBKXJ-38W9R
echo Office Standard 2010 	V7QKV-4XVVR-XYV4D-F7DFM-8R6BM
echo Office SmallBusBasics 2010 	D6QFG-VBYP2-XQHM7-J97RH-VVRCK
echo Project Professional 2010 	YGX6F-PGV49-PGW3J-9BTGG-VHKC6
echo Project Standard 2010 	4HP3K-88W3F-W2K3D-6677X-F9PGB
echo Visio Premium 2010 	D9DWC-HPYVV-JGF4P-BTWQB-WX8BJ
echo Visio Professional 2010 	7MCW8-VRQVK-G677T-PDJCM-Q8TCP
echo Visio Standard 2010 	767HD-QGMWX-8QTDB-9G3R2-KHFGJ
echo Access 2010 	V7Y44-9T38C-R2VJK-666HK-T7DDX
echo Excel 2010 	H62QG-HXVKF-PP4HP-66KMR-CW9BM
echo Groove 2010 	QYYW6-QP4CB-MBV6G-HYMCJ-4T3J4
echo InfoPath 2010 	K96W8-67RPQ-62T9Y-J8FQJ-BT37T
echo OneNote 2010 	Q4Y4M-RHWJM-PY37F-MTKWH-D3XHX
echo Outlook 2010 	7YDC2-CWM8M-RRTJC-8MDVC-X3DWQ
echo PowerPoint 2010 	RC8FX-88JRY-3PF7C-X8P67-P4VTT
echo Publisher 2010 	BFK7F-9MYHM-V68C7-DRQ66-83YTP
echo Word 2010 	HVHB3-C6FV7-KQX9W-YQG79-CRY7T
pause
cls&goto start

:readme
echo 本脚本由Leslie Alexander创作，vlmcsd是Github上Wind4的开源作品，脚本基于vlmcsd创作，旨在简单快捷地部署KMS服务器，让优秀的开源项目易用！
echo 由于本人繁忙，此脚本将不定时更新。如有BUG请联系我！E-mail：leslie@leslieblog.top Telegram:@LeslieALexander
echo 如果你有能力，请赞助我，谢谢！
pause
cls&goto start
