
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// vmm.h for Simple-XX/SimpleKernel.

#ifndef _VMM_H_
#define _VMM_H_

#include "limits.h"
#include "common.h"
#include "pmm.h"
#include "io.h"

typedef void *              page_t;
typedef page_t              page_table_entry_t;
typedef page_table_entry_t *page_table_t;
typedef page_table_t        page_dir_entry_t;
typedef page_dir_entry_t *  page_dir_t;

// 页表项 sizeof(pte_t) 大小 = 4B 2^3
static constexpr const uint32_t VMM_PTE_SIZE = sizeof(page_table_entry_t);

// 每页能映射多少页表项 = 页大小/页表项大小 2^9
static constexpr const uint32_t VMM_PTE_PRE_PAGE =
    COMMON::PAGE_SIZE / VMM_PTE_SIZE;

// 页目录项 sizeof(pgd_t) 大小 = 4B 2^3
static constexpr const uint32_t VMM_PGDE_SIZE = sizeof(page_dir_entry_t);

// 每页能映射多少页目录项 = 页大小/页目录项大小 2^9
static constexpr const uint32_t VMM_PGDE_PRE_PAGE =
    COMMON::PAGE_SIZE / VMM_PGDE_SIZE;

// 每个页表能映射多少页 = 页大小/页表项大小: 2^9
static constexpr const uint32_t VMM_PAGES_PRE_PAGE_TABLE =
    COMMON::PAGE_SIZE / VMM_PTE_SIZE;

// 每个页目录能映射多少页表 = 页大小/页目录项大小 2^9
static constexpr const uint32_t VMM_PAGE_TABLES_PRE_PAGE_DIRECTORY =
    COMMON::PAGE_SIZE / VMM_PGDE_SIZE;

// 每个页目录能映射多少页 = 页表数量*每个页表能映射多少页 2^18
static constexpr const uint32_t VMM_PAGES_PRE_PAGE_DIRECTORY =
    VMM_PAGE_TABLES_PRE_PAGE_DIRECTORY * VMM_PAGES_PRE_PAGE_TABLE;

// 页表大小，一页表能映射多少 Byte 内存 = 页表项数量*页表项映射大小 2^21
static constexpr const uint64_t VMM_PAGE_TABLE_SIZE =
    VMM_PAGES_PRE_PAGE_TABLE * COMMON::PAGE_SIZE;

// 页目录大小，一页目录能映射多少 Byte 内存 = 页表数量*页表映射大小 2^32
static constexpr const uint64_t VMM_PAGE_DIRECTORY_SIZE =
    VMM_PAGE_TABLES_PRE_PAGE_DIRECTORY * VMM_PAGE_TABLE_SIZE;

// 虚拟内存大小 4GB
static constexpr const uint64_t VMM_VMEM_SIZE = UINT_MAX + (uint64_t)1;

// 映射全部虚拟内存需要的页数 = 虚拟内存大小/页大小 2^20
static constexpr const uint64_t VMM_PAGES_TOTAL =
    VMM_VMEM_SIZE / COMMON::PAGE_SIZE;

// 映射全部虚拟内存需要的页表数 = 虚拟内存大小/页表大小 2^12
static constexpr const uint64_t VMM_PAGE_TABLES_TOTAL =
    VMM_VMEM_SIZE / VMM_PAGE_TABLE_SIZE;

// 映射全部虚拟内存需要的页目录数 = 虚拟内存大小/页目录大小 2^0
static constexpr const uint64_t VMM_PAGE_DIRECTORIES_TOTAL =
    (VMM_VMEM_SIZE / VMM_PAGE_DIRECTORY_SIZE) +
    ((VMM_VMEM_SIZE % VMM_PAGE_DIRECTORY_SIZE) == 0 ? 0 : 1);

// 映射内核需要的页表数
static constexpr const uint64_t VMM_PAGE_TABLES_KERNEL =
    (COMMON::KERNEL_SIZE / VMM_PAGE_TABLE_SIZE) +
    (COMMON::KERNEL_SIZE % VMM_PAGE_TABLE_SIZE == 0 ? 0 : 1);

// 映射内核的大小
static constexpr const uint64_t VMM_KERNEL_SIZE = COMMON::KERNEL_SIZE;

// 内核映射的页数
static constexpr const uint64_t VMM_KERNEL_PAGES =
    VMM_KERNEL_SIZE / COMMON::PAGE_SIZE;

// 内核映射的页表数
static constexpr const uint64_t VMM_KERNEL_PAGE_TABLES =
    VMM_KERNEL_SIZE / VMM_PAGE_TABLE_SIZE;

// 内核映射的页目录数
static constexpr const uint64_t VMM_KERNEL_PAGE_DIRECTORIES =
    (VMM_KERNEL_SIZE / VMM_PAGE_DIRECTORY_SIZE) +
    ((VMM_KERNEL_SIZE % VMM_PAGE_DIRECTORY_SIZE) == 0 ? 0 : 1);

// 如果为 0  那么页面只能被运行在超级用户特权级 (0,1 或 2)  的程序访问。
static constexpr const uint32_t VMM_PAGE_KERNEL = 0x00000000;
// P = 1 表示有效； P = 0 表示无效。
static constexpr const uint32_t VMM_PAGE_PRESENT = 0x00000001;
// 如果为 0  表示页面只读或可执行。
static constexpr const uint32_t VMM_PAGE_RW = 0x00000002;
// U/S-- 位 2 是用户 / 超级用户 (User/Supervisor) 标志。
// 如果为 1  那么运行在任何特权级上的程序都可以访问该页面。
static constexpr const uint32_t VMM_PAGE_USER = 0x00000004;

// 获取一个地址的页目录，高 10 位
#define VMM_PGD_INDEX(x) (((x) >> 22) & 0x03FF)

// 获取一个地址的页表，中间 10 位
#define VMM_PTE_INDEX(x) (((x) >> 12) & 0x03FF)

// 获取一个地址的页內偏移，低 12 位
#define VMM_OFFSET_INDEX(x) ((x)&0x0FFF)

// 逻辑地址到物理地址转换
#define VMM_LA_PA(la) (la - COMMON::KERNEL_BASE)

// 物理地址到逻辑地址转换
#define VMM_PA_LA(pa) (pa + COMMON::KERNEL_BASE)

class VMM {
private:
    static IO  io;
    static PMM pmm;
    // 页表
    static page_table_t pte_kernel[VMM_KERNEL_PAGES]
        __attribute__((aligned(COMMON::PAGE_SIZE)));
    // 页目录
    static page_dir_t pgd_kernel[VMM_PAGE_TABLES_TOTAL]
        __attribute__((aligned(COMMON::PAGE_SIZE)));

    page_dir_t curr_dir;

protected:
public:
    VMM(void);
    ~VMM(void);
    // 初始化
    void init(void);
    // 获取当前页目录
    page_dir_t get_pgd(void) const;
    // 设置当前页目录
    void set_pgd(const page_dir_t pgd);
    // 映射物理地址到虚拟地址
    void mmap(const page_dir_t pgd, const void *va, const void *pa,
              const uint32_t flag);
    // 取消映射
    void unmmap(const page_dir_t pgd, const void *va);
    // 获取映射的物理地址
    // 已映射返回 1，未映射返回 0
    uint32_t get_mmap(const page_dir_t pgd, const void *va, const void *pa);
};

#endif /* _VMM_H */
