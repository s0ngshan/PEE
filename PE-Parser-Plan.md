# PE 文件解析器开发计划书（细化版）

说明：这是把你原始计划按周、按任务细化后的可执行计划。每个迭代（版本）目标的工作量控制在 1 周内（5~7 天），同时给出学习要点、实现步骤、验收标准和常见陷阱。

---

## 总说明
- 技术栈：C（解析核心）、MFC（GUI）；开发环境：Visual Studio 2019/2022
- 开发策略：先做稳健的控制台解析核心（File -> Memory -> 解析数据结构 -> 链表存储 -> 单元测试），再把核心以 C 接口暴露给 MFC 界面。
- 常用约定：RVA = 相对虚拟地址，FOA/RAW = 文件内偏移；使用 windows.h 中的 IMAGE_* 结构体或直接拷贝官方定义并加 `#pragma pack(1)`。

---

### V0.0 地基：安全文件读取封装（3 天）
目标：实现稳定、安全的二进制读取封装，为后续内存解析奠定基础。

学习要点：
- C 文件 I/O：fopen/fread/fseek/ftell/fclose
- malloc/free 与边界检查
- 如何获取文件大小并做溢出检测（注意 long 与 size_t 的转换）

任务清单：
1. 实现 FileToMem(const char *path, uint8_t **buf, size_t *size)：将文件一次性读入内存并返回缓冲区与大小。返回值检查并在失败时释放资源。
2. 实现 safe_read_mem(uint8_t *buf, size_t bufsize, size_t offset, void *out, size_t len)：从内存缓冲区安全拷贝，检查越界。
3. 单元验证：读取 calc.exe / notepad.exe 的最后一个字节；尝试读取越界应当返回错误并不崩溃。

验收：示例程序 ./pe_read test.exe 输出文件大小并能正确读出末尾字节。

常见陷阱：不要在 32 位系统下把文件大小放进 32 位 signed int，注意使用 size_t / uint64_t 做保底。

---

### V1.0 核心战：获取入口点 RVA（4 天）
目标：实现最小可用的 PE 解析，输出 EntryPoint RVA。

学习要点：
- IMAGE_DOS_HEADER 的 e_lfanew 字段含义
- NT headers 的 Signature 验证（"PE\0\0"）
- OptionalHeader 中的 AddressOfEntryPoint 字段
- #pragma pack(1) 或使用 windows.h 的结构（注意对齐）

任务清单：
1. 在内存缓冲区上实现 GetDosHeader(buf) / GetNtHeaders(buf) 的安全访问（用 safe_read_mem 风格校验）。
2. 从 buf[0x3C] 读取 e_lfanew，跳转到该偏移并校验 "PE\0\0"。
3. 读取 OptionalHeader.AddressOfEntryPoint 并打印十六进制结果。
4. 小工具：pe_entry <path>

验收：对 notepad.exe、calc.exe 输出与 PEview/PE Explorer 一致的 EntryPoint RVA。

常见陷阱：在 64 位 PE（PE32+）与 32 位 PE（PE32）中 OptionalHeader 的结构不同，AddressOfEntryPoint 字段的位置相同但 OptionalHeader 大小不同，要分别处理（检查 Magic 字段 0x10B / 0x20B）。

---

### V1.5 工程化：完整 NT 头与节表（5 天）
目标：解析并打印完整的 FileHeader / OptionalHeader 字段，并遍历节表，将节信息存入链表结构。

学习要点：
- IMAGE_FILE_HEADER 与 IMAGE_OPTIONAL_HEADER 的关键字段含义（Machine / NumberOfSections / SizeOfImage / ImageBase）
- IMAGE_SECTION_HEADER 的字段（Name, VirtualAddress, SizeOfRawData, PointerToRawData）
- 链表（单向链表）在 C 中的实现和内存管理

任务清单：
1. 定义 SectionNode 结构：包含 IMAGE_SECTION_HEADER (+ 保存原始字节拷贝) 与 next 指针。
2. 解析 NumberOfSections 并按顺序读取每个 IMAGE_SECTION_HEADER，malloc 一个节点 append 到链表。
3. 实现 PrintNTHeaders 和 PrintSectionList 函数输出可读文本。
4. 加入命令行参数 `-v` 输出更详细信息（ImageBase、SizeOfImage、Characteristics 等）。

验收：控制台程序完整打印 NT headers 信息并以列表形式打印所有节（名称、RVA、RawPtr、Size）。

常见陷阱：节名可能不是以 '\0' 结尾（8 字节固定），打印时注意按长度处理；PointerToRawData 为 0 时小心处理（某些节在文件中没有数据）。

---

### V2.0 MFC 框架搭建 + 树控件渲染（4 天）
目标：把控制台解析器以 C 接口形式暴露，创建基于对话框的 MFC 程序，并用 CTreeCtrl 展示解析结果树形结构。

学习要点：
- Visual Studio 新建 MFC 对话框程序
- C 与 C++ 混编：在头文件用 extern "C" 包裹 C 函数声明
- CTreeCtrl 的 InsertItem、SetItemData、Expand 等常用函数
- Unicode <-> ANSI 转换（TCHAR / CString / CA2W）

任务清单：
1. 新建 MFC Dialog 项目（Empty 项目模板 + 对话框），在对话框上放置 CTreeCtrl 和 Open 按钮。
2. 将 pe_parser.c / pe_parser.h 加入项目（保持 C 编译），在 pe_parser.h 中声明 `extern "C"` 接口：ParsePE(const uint8_t *buf, size_t size, PE_INFO **out)`。
3. 点击“打开文件”后调用 FileToMem + ParsePE，ParsePE 返回的结构（或链表）在 MFC 中被遍历并 InsertItem。
4. 每个树节点用 SetItemData 存储对应的文件偏移（或结构指针），以便后续联动。

验收：能用 GUI 打开 exe，左侧树控件展示 DOS Header、NT Headers、Section Headers 等分支，能展开查看字段名与值。

常见陷阱：MFC 项目默认 UNICODE，注意字符串传递；避免在 GUI 线程做长时间解析（对大文件可考虑单独线程，但初版可以同步完成）。

---

### V2.1 树形细化与图标美化（3 天）
目标：细化树节点字段显示，为关键节点添加图标，提高可读性。

学习要点：
- CImageList 与 CTreeCtrl::SetImageList
- TreeView 的 TVITEM 结构及图标索引管理
- 如何把关键字段（例如 e_lfanew）作为子节点显示并附带偏移信息

任务清单：
1. 准备 2~4 个小图标（header、section、dll、func），加载到 CImageList 并关联到 TreeCtrl。
2. 为每个 Section 节点添加子节点：VirtualAddress / SizeOfRawData / PointerToRawData，并在节点文本中添加偏移（例如 "PointerToRawData: 0x1234"）。
3. 为每个插入的节点用 SetItemData 存储对应文件偏移（uint32 或指针），以便右侧十六进制视图联动。

验收：界面美观，树形能展开显示所有字段且有图标，节点上能看到偏移数字。

---

### V2.2 十六进制查看器与联动（7 天）
目标：实现右侧十六进制视图（每行 16 字节）并支持树控件点击后定位并高亮相应字节区域。

学习要点：
- CListCtrl（Report 模式 / 虚拟模式）或自绘方式（更灵活但复杂）
- 如何实现一个高效的十六进制视图：按行渲染地址、16 字节 hex、ASCII
- ListCtrl 的 EnsureVisible / SetItemState 用于高亮
- 使用 SetItemData 在树节点和 hex 视图间传递偏移信息

任务清单：
1. 在对话框右侧放置 CListCtrl，列：Address | Hex (16 bytes) | ASCII
2. 将整个文件缓冲区按行填充到 ListCtrl：第 0 行表示偏移 0x0 的 16 字节，第 N 行地址 = N * 16。
3. 当树节点被选择（TVN_SELCHANGED），读取 ItemData（偏移 + 长度），计算目标行和列并调用 EnsureVisible、SetItemState 标记高亮。
4. 实现简单的高亮：把目标行背景或选中状态改为蓝色；如需多个字节跨行高亮，处理分段高亮。

验收：点击 e_lfanew 节点后，右侧定位到文件偏移 0x3C 并高亮 4 字节内容（显示正确的值）。

常见陷阱：ListCtrl 在项目太多时性能下降，若遇到卡顿考虑使用虚拟列表（LVS_OWNERDATA）或直接自绘（OnPaint）。

---

### V3.0 导入表 / 导出表解析（7 天）
目标：解析 DataDirectory 中的导入表（IMAGE_DIRECTORY_ENTRY_IMPORT），在树中展示 DLL 与函数名（区分按序号导入与按名导入）。

学习要点：
- OptionalHeader.DataDirectory 结构理解
- IMAGE_IMPORT_DESCRIPTOR 的字段（OriginalFirstThunk / Name / FirstThunk）
- IMAGE_THUNK_DATA 的两种含义：导入按序号（最高位为 1）与按名（指向 IMAGE_IMPORT_BY_NAME）
- 字符串读取与 FOA/RVA 转换（调用已实现的 RvaToFoa）

任务清单：
1. 在解析核心实现 FindDataDirectory(IMAGE_DIRECTORY_ENTRY_IMPORT) 并拿到 RVA+Size。
2. 使用 RvaToFoa 找到 IMAGE_IMPORT_DESCRIPTOR 数组在文件中的起始 FOA，遍历直到全部 0 结构。
3. 对每个 descriptor，读取 DLL 名称（RVA -> FOA -> 字符串），然后遍历 INT/IAT，区分按名/按序号，按名则读取 IMAGE_IMPORT_BY_NAME 结构并取得函数名。
4. GUI：在树中加入 Import Table 分支，DLL 名为父节点，函数名为子节点；子节点同时保存 IAT 在文件中的偏移以便右侧高亮。

验收：打开常见 exe（notepad.exe），能看到 KERNEL32.dll、ntdll.dll（如存在），及 CreateFileW、ReadFile 等函数名。

常见陷阱：PE 的导入表可能经过重定位或有 IAT 被修补过（例如加壳/运行时修改），静态解析时可能与运行时 IAT 不一致；处理 32/64 位 thunk 格式差异（IMAGE_THUNK_DATA64 vs IMAGE_THUNK_DATA32）。

---

## 附加建议（工具与资料）
- 编辑器/IDE：Visual Studio 2019/2022（带 MFC 支持）
- 参考资料：Microsoft PE/COFF specification、ReactOS 源码（对 IMAGE_ 结构的注释很有用）、Charles Leaver 的《PE-format》教程
- 调试工具：PEview、PE-bear、CFF Explorer、x64dbg（动态验证）
- 小建议：先把解析器写成只读的、不可变的函数（不改输入缓冲区），后续再做修改类功能（IAT 修复、重写头等）。

---

## 项目交付（本计划附带文件）
- 建议在项目根目录创建：
  - /src/pe_parser.c, /include/pe_parser.h
  - /mfc/（MFC GUI 项目）
  - PE-Parser-Plan.md （本文件）

祝好，按此计划逐周推进即可把原型做成可演示的 GUI 工具。若需要，我可以根据你已有代码把解析核心（RVA->FOA、节表链表、导入表解析）补全为可编译的 C 源文件。
