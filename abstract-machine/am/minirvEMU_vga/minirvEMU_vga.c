#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <am.h>        
#include <amdev.h>     

// ==========================================
// 常量、宏定义与全局显存
// ==========================================
#define MEM_SIZE (16 * 1024 * 1024)  // 16MB 主存
#define INST_EBREAK 0x00100073       // ebreak 指令
#define INST_EMPTY  0x00000000       // 空指令

#define FB_ADDR 0x20000000           // 显存映射基址
#define FB_SIZE 0x00040000           // 显存大小 (256KB)

// 全局显存数组，每个元素为一个32位像素 (ARGB)
uint32_t vmem[FB_SIZE / 4];

// ==========================================
// 结构体定义
// ==========================================
typedef struct {
    uint32_t regs[32]; // 通用寄存器，x10 = a0
    uint32_t pc;       // 程序计数器
    uint8_t *mem;      // 主存指针
} rv_cpu_t;

typedef struct {
    uint32_t opcode;
    uint32_t rd;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t funct3;
    uint32_t funct7;
    int32_t  imm;      
    int32_t  imm_s;    
} instruction_t;

// ==========================================
// 核心CPU函数实现
// ==========================================
// 从内存读取一个32位字
uint32_t load_word(rv_cpu_t *cpu, uint32_t addr) {
    if (addr >= MEM_SIZE - 3) return 0;
    return cpu->mem[addr] | 
          (cpu->mem[addr + 1] << 8) | 
          (cpu->mem[addr + 2] << 16) | 
          (cpu->mem[addr + 3] << 24);
}

// 取指
uint32_t fetch(rv_cpu_t *cpu, uint32_t pc) {
    return load_word(cpu, pc);
}

// 指令解码
instruction_t decode(uint32_t inst) {
    instruction_t ins = {0};
    ins.opcode = inst & 0x7F;
    ins.rd     = (inst >> 7) & 0x1F;
    ins.funct3 = (inst >> 12) & 0x07;
    ins.rs1    = (inst >> 15) & 0x1F;
    ins.rs2    = (inst >> 20) & 0x1F;
    ins.funct7 = (inst >> 25) & 0x7F;
    
    ins.imm    = ((int32_t)inst) >> 20; 
    ins.imm_s  = ((int32_t)(inst & 0xFE000000) >> 20) | ((inst >> 7) & 0x1F);
    
    return ins;
}

// 指令执行
void execute(rv_cpu_t *cpu, instruction_t ins) {
    cpu->regs[0] = 0; // x0恒为0
    uint32_t next_pc = cpu->pc + 4;

    switch (ins.opcode) {
        case 0x13: // ADDI 立即数加法
            if (ins.funct3 == 0x0) { 
                if (ins.rd != 0) 
                    cpu->regs[ins.rd] = cpu->regs[ins.rs1] + ins.imm;
            }
            break;
            
        case 0x33: // ADD 寄存器加法
            if (ins.funct3 == 0x0 && ins.funct7 == 0x00) { 
                if (ins.rd != 0) 
                    cpu->regs[ins.rd] = cpu->regs[ins.rs1] + cpu->regs[ins.rs2];
            }
            break;
            
        case 0x23: // SW 存字指令
            if (ins.funct3 == 0x2) { 
                uint32_t addr = cpu->regs[ins.rs1] + ins.imm_s;
                uint32_t val  = cpu->regs[ins.rs2];

                // MMIO：写入VGA显存
                if (addr >= FB_ADDR && addr < FB_ADDR + FB_SIZE) {
                    uint32_t offset = (addr - FB_ADDR) / 4;
                    vmem[offset] = val;
                } 
                // 写入物理内存
                else if (addr < MEM_SIZE - 3) {
                    cpu->mem[addr]     = val & 0xFF;
                    cpu->mem[addr + 1] = (val >> 8) & 0xFF;
                    cpu->mem[addr + 2] = (val >> 16) & 0xFF;
                    cpu->mem[addr + 3] = (val >> 24) & 0xFF;
                }
            }
            break;
    }
    cpu->pc = next_pc;
}

// ==========================================
// 加载二进制文件到内存
// ==========================================
int load_binary_file(rv_cpu_t *cpu, const char *filename, uint32_t base_addr) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "错误：无法找到文件 '%s'\n", filename);
        return -1;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 越界检查
    if (base_addr >= MEM_SIZE || file_size > (MEM_SIZE - base_addr)) {
        fclose(file);
        fprintf(stderr, "错误：文件超出内存范围\n");
        return -1;
    }

    size_t bytes_read = fread(&cpu->mem[base_addr], 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "错误：文件读取不完整\n");
        return -1;
    }
    printf("成功加载 %s (%ld 字节)\n", filename, file_size);
    return 0;
}

// ==========================================
// 主函数
// ==========================================
int main(int argc, char *argv[]) {
    ioe_init(); // 初始化AM环境
    
    rv_cpu_t cpu;
    memset(cpu.regs, 0, sizeof(cpu.regs));
    cpu.pc = 0;
    cpu.mem = (uint8_t *)calloc(1, MEM_SIZE);
    memset(vmem, 0, sizeof(vmem));
    
    // 固定加载vga.bin
    const char* target_file = "vga.bin";
    uint32_t base_addr = 0;
    
    // 支持命令行传入加载地址
    if (argc >= 2) {
        base_addr = (uint32_t)strtoul(argv[1], NULL, 16);
    }
    
    // 加载失败则退出
    if (load_binary_file(&cpu, target_file, base_addr) != 0) {
        free(cpu.mem);
        return 1;
    }
    
    // 初始化CPU
    cpu.pc = base_addr;
    cpu.regs[2] = MEM_SIZE - 4; // 设置栈指针SP
    
    // 植入ebreak指令，用于程序正常结束
    uint32_t halt_addr = 0x1044; 
    if (halt_addr < MEM_SIZE - 3) {
        cpu.mem[halt_addr + 0] = 0x73;
        cpu.mem[halt_addr + 1] = 0x00;
        cpu.mem[halt_addr + 2] = 0x10;
        cpu.mem[halt_addr + 3] = 0x00;
    }

    // 执行指令
    printf("开始执行，最大周期：500000\n");
    uint64_t cycle = 0;
    const uint64_t MAX_CYCLES = 500000; 
    
    while (cycle < MAX_CYCLES) {
        // 非法PC直接退出
        if (cpu.pc >= MEM_SIZE - 3 || cpu.pc % 4 != 0) {
            fprintf(stderr, "错误：PC非法 0x%08x\n", cpu.pc);
            break;
        }

        uint32_t inst = fetch(&cpu, cpu.pc);
        
        // 程序结束指令
        if (inst == INST_EBREAK) {
            if (cpu.regs[10] == 0) 
                printf("\n✅ 执行成功：HIT GOOD TRAP\n");
            else 
                printf("\n❌ 执行失败：HIT BAD TRAP (a0=%d)\n", cpu.regs[10]);
            break;
        }
        
        // 遇到空指令停止
        if (inst == INST_EMPTY) {
            printf("⚠️  遇到空指令，停止执行\n");
            break;
        }
        
        instruction_t ins = decode(inst);
        execute(&cpu, ins);
        cycle++;
    }
    printf("执行周期数：%llu\n", (unsigned long long)cycle);
    
    // ==========================================
    // 渲染显存内容到屏幕
    // ==========================================
    printf("\n开始渲染图像...\n");

    AM_GPU_CONFIG_T cfg;
    ioe_read(AM_GPU_CONFIG, &cfg);
    
    AM_GPU_FBDRAW_T ctl;
    ctl.x = 0;
    ctl.y = 0;
    ctl.pixels = vmem;
    ctl.w = cfg.width;
    ctl.h = cfg.height;
    ctl.sync = true;
    ioe_write(AM_GPU_FBDRAW, &ctl);

    printf("✅ 渲染完成，窗口已打开\n");
    while (1); // 保持窗口
    
    free(cpu.mem);
    return 0;
}

