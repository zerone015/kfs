# KFS (Kernel From Scratch)

x86 기반의 Unix 스타일 커널 구현 프로젝트입니다.  
커널을 직접 설계하고 구현하여 추상적인 이해를 넘어서는 것을 목표로 합니다.

## 실행 방법

필수 도구
- GCC 크로스 컴파일러 ([OSDev Wiki 참고](https://wiki.osdev.org/GCC_Cross-Compiler))
- NASM (어셈블리 빌드)
- QEMU (에뮬레이터)

빌드 및 실행
```bash
make
qemu-system-i386 -kernel kernel.elf -m <RAM_SIZE>
```

## 목차

- [0. 부팅](#0-부팅)
  - [전체 흐름](#전체-흐름)
  - [_start](#_start)
  - [.higher_half](#higher_half)
  - [kmain](#kmain)
- [1. 메모리 관리](#1-메모리-관리)
  - [물리 메모리](#물리-메모리)
  - [가상 주소 공간](#가상-주소-공간)
  - [힙 영역](#힙-영역)
- [2. 시스템 콜 인터페이스](#2-시스템-콜-인터페이스)
  - [fork](#1-fork)
  - [_exit](#2-_exit)
  - [waitpid](#3-waitpid)
  - [kill](#4-kill)
  - [signal](#5-signal)
  - [sigreturn](#6-sigreturn)
  - [시그널 처리 흐름](#시그널-처리-흐름)
- [3. 인터럽트](#3-인터럽트)
  - [예외](#예외)
  - [하드웨어 인터럽트](#하드웨어-인터럽트)
- [4. 스케줄링](#4-스케줄링)
- [테스트 및 검증](#테스트-및-검증)


## 설계 및 구현

### 0. 부팅

#### 전체 흐름

```
BIOS  →  GRUB  →  _start  →  .higher_half  →  kmain
```

#### [`_start`](https://github.com/zerone015/kfs/blob/master/src/boot/boot.asm#L48)

`.multiboot.text` 섹션에 위치하며 물리 주소 기반으로 실행되는 커널 진입점이다.  
페이징을 활성화하고 `.higher_half`로 넘어가기까지 다음 순서로 진행된다.

##### 페이지 디렉터리 구성

엔트리 768, 769번에 커널을 상위 절반(`0xC0000000`)으로 매핑하고, 추가로 네 가지 매핑을 설정한다.

- Identity 매핑 (엔트리 0, 1)  
  페이징 활성화 직후 EIP는 여전히 물리 주소를 가리키므로, 매핑이 없으면 트리플 폴트가 발생한다. 이를 방지하기 위해 0~8MiB 영역을 동일한 가상 주소로 임시 매핑해둔다. `.higher_half` 진입 후 두 엔트리를 초기화하고 CR3을 재로드해 제거된다.

- multiboot_info 임시 매핑 (엔트리 770)  
  GRUB이 전달한 `ebx`의 물리 주소가 속한 4MB 영역을 `0xC0800000`에 매핑한다. 이후 `ebx`를 `0xC0800000 | (ebx & 0x3FFFFF)` 방식으로 변환해 `kmain`에 가상 주소로 전달한다. 이 매핑은 물리 메모리 초기화 단계에서 제거된다.

- 임시 4KB 매핑 영역 (엔트리 1022)  
  커널 공간은 4MB 페이지를 사용하지만, COW 처리 시 유저 페이지 복제나 `fork` 시 페이지 테이블 복제처럼 4KB 페이지를 커널 공간에 임시로 매핑해야 하는 경우가 있다. 이를 위해 해당 영역을 예약해두고, 필요 시 매핑 후 작업 완료 시 해제하는 방식으로 사용한다.

- 재귀적 페이징 설정 (엔트리 1023)  
  페이지 디렉터리의 마지막 엔트리에 자기 자신의 물리 주소를 저장한다.  
  이후 MMU의 주소 변환 메커니즘을 통해 별도 매핑 없이 페이지 테이블과 디렉터리에 직접 접근할 수 있게 된다.  
  자세한 내용은 [페이지 테이블 관리](#페이지-테이블-관리) 섹션을 참고한다.

##### Control Registers 설정

CR4에 PSE(4MB 페이지)와 PGE(글로벌 페이지) 비트를 set한다.  
CR3에 페이지 디렉터리를 로드하고, CR0에 PG(페이징), WP(슈퍼바이저 쓰기 보호), NE(x87 FP 예외를 #MF로 처리) 비트를 set해 페이징을 활성화한다.  
이후 `jmp .higher_half`로 상위 절반 실행으로 전환한다.

#### [`.higher_half`](https://github.com/zerone015/kfs/blob/master/src/boot/boot.asm#L48)

`.text` 섹션에 위치하며, 링커 스크립트에 의해 가상 주소로 재배치된다. 여기서부터 가상 주소 기반으로 실행된다. Identity 매핑을 제거하고 `kmain(magic, mbi)`을 호출한다.

#### [`kmain`](https://github.com/zerone015/kfs/blob/master/src/kernel/main.c#L55)

페이징이 활성화된 이후 진입하는 C 진입점이다.  
각 단계는 다음 단계의 전제가 된다.

```
kmain()
├── arch_init()         # VGA, TTY, GDT, IDT, PIC 초기화
├── bootloader_check()  # 멀티부트 정보 유효성 검사
├── memory_init()       # 물리 메모리, 가상 주소 공간, 힙 초기화
├── scheduler_init()    # PIT, TSS 초기화
└── process_init()      # 프로세스 자료구조 초기화 및 init 프로세스 생성
```

`process_init()`에서 생성되는 init 프로세스는 유저 공간을 직접 구성해 테스트 코드를 실행하는 역할을 맡는다.


### 1. 메모리 관리

### 물리 메모리

부팅 시 `multiboot_info`에 포함된 메모리 맵을 통해 사용 가능한 물리 메모리 영역을 식별하고,  
이를 버디 할당 시스템에 등록하여 관리한다.

다중 페이지 크기를 지원하기 위해 버디 시스템을 채택했다.  
구현은 두 단계를 거쳐 발전했다.

| 방식 | 시간 복잡도 | 다중 페이지 크기 지원 |
|------|-------------|------------------------|
| 스택 기반 | `O(1)` | X |
| 비트맵 기반 | `O(N)` | O |
| 비트맵 기반 버디 시스템 | `O(N)` | O |
| free list 기반 버디 시스템 (현재) | `O(1)` / `O(log n)` | O |

#### 1단계: [비트맵 기반 버디 시스템](https://github.com/zerone015/kfs/blob/df24f0115030f61fb19758ce47ebe31dc1d9f112/src/pmm.c#L244)

```c
struct buddy_order {
    uint32_t *bitmap;
    size_t free_count;
};

struct buddy_allocator {
    struct buddy_order orders[MAX_ORDER];
};
```

오더마다 비트맵을 두고, 각 비트의 오프셋이 해당 오더 크기 페이지의 PFN을 나타낸다.  
오더가 증가할수록 비트맵 크기는 절반으로 줄어든다.  
비트맵 접근 단위를 4바이트로 두어 탐색 횟수를 어느 정도 줄였지만, 페이지 할당자로 쓰기에는 여전히 O(N) 선형 탐색 비용이 발생했다.

- alloc_pages: `free_count`가 있는 오더를 찾은 뒤 비트맵을 순회해 free 비트를 클리어한다. 상위 오더에서 찾은 경우 하위 오더로 반복 분할하며, 분할 시 하위 오더의 오프셋은 현재 오프셋에 2를 곱해 구한다. 분할은 항상 쌍의 오른쪽 페이지를 반복적으로 나누며, 최종적으로 할당되는 것은 쌍의 왼쪽 페이지가 된다. 버디 오프셋은 현재 오프셋에 1을 XOR해 구한다.
- free_pages: 버디가 free 상태면 병합한다. 상위 오더의 오프셋은 현재 오프셋을 2로 나눠 구하며, 병합할 수 없을 때까지 오더를 타고 올라간다.

#### 2단계: [free list 기반 버디 시스템 (현재)](https://github.com/zerone015/kfs/blob/master/src/kernel/pmm.c#L434)

Linux에서 아이디어를 얻어 설계했다.

```c
struct page {
    uint32_t order;
    uint32_t ref;
    uint32_t flags;
    struct list_head free_list;
};

struct buddy_order {
    struct list_head free_list;
};

struct buddy_allocator {
    struct buddy_order orders[MAX_ORDER];
};
```

비트맵을 제거하고 각 오더마다 free list를 유지한다.  
각 물리 페이지의 메타데이터를 담는 `struct page`가 추가되었으며,  
이 구조체들을 PFN으로 인덱싱하는 배열 `pgmap`이 핵심 역할을 한다.

pgmap  
물리 메모리 초기화 시 파악한 RAM 크기를 기준으로 원소 수가 정해진다.  
4GB RAM이라면 최대 1MB개의 4KB 페이지가 존재하므로 배열도 1MB개 원소를 갖는다.  
페이지 블록의 가장 낮은 PFN(base PFN)이 해당 블록의 대표 식별자이며,  
`pgmap[base_pfn]`으로 O(1) 접근이 가능하다.

alloc_pages  
할당 가능한 오더를 찾으면 free list의 head에서 바로 `struct page`를 꺼낸다.  
기존 O(N) 순회가 O(1)로 줄어든 부분이다. 상위 오더에서 분할이 필요한 경우 전체 시간복잡도는 O(log n)이 된다.  

분할 시 항상 쌍의 왼쪽을 계속 나누도록 설계했다.  
왼쪽을 반복 분할하면 base PFN이 변하지 않아 재계산이 필요 없기 때문이다.  
오른쪽 버디의 PFN은 현재 base PFN에 bpages(현재 오더의 페이지 수)를 더해 구한다.

free_pages  
버디의 PFN은 base PFN에 bpages를 XOR하여 구한다.  
base PFN은 항상 bpages의 배수로 정렬되어 있으므로 XOR 연산으로 쌍의 반대편 PFN을 정확히 찾을 수 있다.  
`pgmap[buddy_pfn]`으로 버디 메타데이터에 O(1) 접근 후 병합 가능 여부를 확인하며,  
병합 시 상위 오더의 base PFN은 현재 PFN을 `bpages * 2`로 내림 정렬하여 구한다.

#### 초기화 흐름

[`pmm_init()`](https://github.com/zerone015/kfs/blob/master/src/kernel/pmm.c#L518)에서 시작하며 다음 순서로 진행된다.

```
pmm_init()
├── mmap_copy_from_grub()   # 멀티부트 메모리 맵 복사
├── mmap_trim_highmem()     # 4GB 초과 영역 제거
├── mmap_reserve_kernel()   # 커널 적재 영역 reserved 처리
├── mmap_align()            # 페이지 크기로 정렬
└── __pmm_init()            # 버디 할당자 초기화
```

- mmap_copy_from_grub: 멀티부트 메모리 맵은 물리 주소에 위치하며, GRUB이 BIOS로부터 수집할 당시 해당 영역은 사용 가능으로 보고된 영역이다. 따라서 덮어씌워질 수 있다. 이 데이터는 초기화 함수 내에서만 참조되므로, 별도로 reserved 처리하기보다는 스택 프레임에 복사한 뒤 사용하고 버리는 방식이 더 단순하다. 원본 영역은 그대로 사용 가능 상태로 유지한다.
- mmap_trim_highmem: PAE를 지원하지 않으므로 4GB 이상의 물리 메모리는 접근 불가하다. 이 영역을 잘라낸다.
- mmap_reserve_kernel: 커널이 적재된 영역도 사용 가능으로 보고된다. 해당 물리 주소 범위를 reserved로 변경한다.
- mmap_align: 메모리 맵의 영역 경계는 페이지 정렬이 보장되지 않는다. 각 영역의 시작과 끝을 페이지 크기에 맞게 정렬한다.
- __pmm_init: 버디 할당자를 초기화한다. 메타데이터 영역을 확보하고, 사용 가능한 영역들을 페이지 단위로 버디 할당자에 등록한다.

#### 페이지 크기 정책

사용자 공간은 4KB, 커널 공간은 4MB 페이지를 사용한다.

4MB 페이지는 내부 단편화가 있지만, 커널은 스왑되지 않으므로 단편화된 영역이 디스크 탐색 지연과 회전 지연을 유발하지 않는다.
또한 커널의 단일 페이지 낭비는 전체 메모리에서 차지하는 비중이 작고, 페이지 테이블 제거와 메타데이터 절감으로 얻는 이득이 이를 상쇄한다.

4MB 페이지의 주요 장점은 다음과 같다.

- 가상 주소 관리 오버헤드 감소: 4KB 페이지 대비 1024배 큰 영역을 단일 엔트리로 관리하므로 페이지 테이블 엔트리 수가 1024배 감소한다.
- 페이지 워크 성능 향상: 2단계 페이징 구조 중 페이지 테이블 단계를 생략하여 메모리 참조 횟수가 줄어든다.
- TLB 효율 향상: 일부 CPU는 4KB와 4MB TLB를 별도 관리하여 유저 공간 접근이 커널 TLB 엔트리를 밀어내지 않는다. 통합형 TLB에서도 1024배 넓은 영역을 단일 엔트리로 관리해 TLB 미스율이 낮다.


### 가상 주소 공간

상위 절반 커널(Higher Half Kernel) 모델을 채택했다.  
모든 프로세스의 가상 주소 공간 상위 절반(`0xC0000000 ~ 0xFFFFFFFF`)에 커널이 위치한다.

커널을 상위에 고정하면 커널 크기가 커지더라도 유저 프로세스의 시작 주소가 항상 `0x00000000`으로 유지된다.  
또한 32비트 주소 공간 전체를 유저에게 줄 수 있고, Unix 계열 OS의 전통적인 관례를 따라 기존 코드 포팅이 수월하다.  
커널 페이지는 글로벌 페이지로 설정하여 문맥 교환 시에도 커널 영역의 TLB 엔트리가 flush되지 않는다.

#### 페이지 테이블 관리

페이지 테이블에 접근하려면 해당 테이블이 가상 주소 공간에 매핑되어 있어야 한다.  
각 프로세스는 고유한 페이지 테이블을 가지고 있는데, 커널 가상 공간은 약 1GB로 제한되어 모든 프로세스의 페이지 테이블을 동시에 매핑할 수 없다.  
필요할 때마다 매핑/언매핑하는 방식은 매번 TLB 무효화가 필요해 성능 오버헤드가 발생한다.

재귀적 페이징으로 이를 해결한다.  
페이지 디렉터리의 마지막 엔트리(1023)에 자기 자신의 물리 주소를 저장하면,  
MMU의 주소 변환 메커니즘을 통해 별도 매핑 없이 페이지 테이블과 디렉터리에 직접 접근할 수 있다.

```
0xFFC00000  →  현재 프로세스의 첫 번째 페이지 테이블 엔트리
0xFFFFF000  →  현재 프로세스의 첫 번째 페이지 디렉터리 엔트리
```

가상 주소와 PTE/PDE 주소 간 변환은 다음 비트 연산으로 처리한다.

```c
/* addr → pte */
pte = 0xFFC00000
    | ((addr & 0xFFC00000) >> 10)
    | ((addr & 0x003FF000) >> 10);

/* addr → pde */
pde = 0xFFFFF000 | ((addr & 0xFFC00000) >> 20);

/* pte → addr */
addr = ((pte & 0x003FF000) << 10)
     | ((pte & 0x00000FFC) << 10);
```

약 4MB의 가상 공간을 희생하지만, 접근 과정이 단순해져 관리 효율이 크게 향상된다.

#### 커널 공간 관리

| 항목 | 내용 |
|------|------|
| 주소 범위 | `0xC0000000 ~ 0xFFFFFFFF` |
| 페이지 크기 | 4MB (총 256개) |
| 자료구조 | Linked List |

커널은 4MB 페이지 256개를 사용하므로, 최악의 경우에도 관리해야 할 블록 수는 약 128개 수준이다.  
(연속된 가상 블록 사이에 할당된 블록이 끼어 free 블록들이 병합되지 못하는 비정상적인 패턴에서만 나타난다.)  
일반적인 커널 메모리 사용에서는 이런 단편화가 거의 발생하지 않으므로 Linked List만으로 충분히 효율적이다.

##### 초기화

[`vmm_init()`](https://github.com/zerone015/kfs/blob/master/src/kernel/vmm.c#L188)은 두 함수를 호출한다.

```
vmm_init()
├── vb_init()        # 커널 가용 가상 블록 등록
└── vb_pool_init()   # 가상 블록 구조체 풀 초기화
```

`vb_init()`은 커널이 매핑된 공간과 임시 매핑 영역 사이의 가용 공간을 하나의 큰 블록으로 만들어 free list에 등록한다.  
`vb_pool_init()`은 가상 블록 구조체를 빠르게 가져오기 위한 스택 기반 풀을 초기화한다. 최대 개수만큼 배열에 미리 할당되어 있으며, 미사용 구조체들이 풀에 들어간다.

##### 할당 및 해제

[`vb_alloc()`](https://github.com/zerone015/kfs/blob/master/src/kernel/vmm.c#L145)은 free list에서 최초 적합(first fit) 방식으로 블록을 탐색하고, 요청보다 큰 블록을 찾으면 분할해 반환한다.

가상 공간만 할당되며 물리 페이지는 즉시 할당되지 않는다. 실제 접근 시 페이지 폴트 핸들러에서 지연 할당된다. x86 PDE의 자유 비트를 `PG_RESERVED`로 활용해 의도된 폴트인지 구분하며, `PG_CONTIGUOUS` 비트로 PDE 상에서 가상 블록의 끝을 식별한다.

[`vb_free()`](https://github.com/zerone015/kfs/blob/master/src/kernel/vmm.c#L137)는 두 함수를 순서대로 호출한다.

```
vb_free()
├── vb_size_with_free()   # PDE 순회, 물리 페이지 회수 및 TLB 무효화
└── vb_add_and_merge()    # free list 재삽입 및 인접 블록 병합
```

`vb_size_with_free()`는 가상 주소를 PDE 접근 주소로 역산 후 PDE를 순회한다. 가상 공간만 예약된 경우는 한 번도 접근되지 않은 것이므로 TLB 무효화가 불필요하다.  
`vb_add_and_merge()`는 해제된 블록을 free list에 재삽입하고 인접 블록과 병합을 시도한다. 결과적으로 주소 기준 오름차순 정렬된다.

#### 유저 공간 관리

| 항목 | 내용 |
|------|------|
| 주소 범위 | `0x00000000 ~ 0xBFFFFFFF` |
| 페이지 크기 | 4KB (총 786,432개) |
| 자료구조 | Intrusive Red-Black Tree (2-way keyed) |

유저 공간은 최대 약 40만 개의 가상 블록을 관리해야 하므로 Linked List만으로는 효율적인 탐색이 어렵다.

단일 키 트리의 한계를 극복하기 위해 주소 기반 트리와 크기 기반 트리 두 개를 동시에 유지한다.

| 키 기준 | 장점 | 단점 |
|----------|------|------|
| 주소 | 해제(병합) 빠름 | 할당 시 느림 |
| 크기 | 할당 빠름 | 해제(병합) 느림 |

Intrusive Data Structure 형태로 구현하여 메모리 중복과 삭제 오버헤드를 최소화했다.

유저 메모리는 익명 메모리와 파일 매핑 메모리로 나뉘며, 파일 매핑은 참조 관리나 쓰기 반영 같은 추가 처리가 필요하다. 이 때문에 할당된 블록을 별도 자료구조로 추적한다.

##### 초기화

유저 가상 공간은 부팅 시 초기화되지 않는다. 메타데이터는 [`task_struct`](https://github.com/zerone015/kfs/blob/master/include/sched_defs.h#L28)에 있으며 `fork` 시 복제된다.  
`execve`는 아직 미구현이나, 유사한 역할을 하는 [`exec_fn()`](https://github.com/zerone015/kfs/blob/master/src/kernel/exec.c#L128)이 있다.

##### 클린업

[`user_vas_cleanup()`](https://github.com/zerone015/kfs/blob/master/include/vmm.h#L165)은 `exec_fn`과 `_exit`에서 호출된다.

```
user_vas_cleanup()
├── vblocks_cleanup()          # 할당 가능한 가상 블록 구조 정리
├── mapped_vblocks_cleanup()   # 할당된 가상 블록 구조 정리 및 물리 페이지 회수
└── pgdir_cleanup()            # 페이지 테이블 해제
```

코드 재사용과 불필요한 동작 제거를 동시에 달성하기 위해 `__attribute__((always_inline)`을 적용하고 컴파일러의 정적 분기 제거를 활용한다.  
세 개의 플래그(`CL_TLB_INVL`, `CL_RECYCLE`, `CL_MAPPING_FREE`)로 호출 맥락에 따라 동작을 제어한다.

#### 지연 할당

가상 공간 할당 시 물리 메모리는 즉시 할당되지 않는다.  
해당 공간에 실제 접근이 발생했을 때 [페이지 폴트 핸들러](https://github.com/zerone015/kfs/blob/master/src/kernel/isr.c#L115)가 물리 페이지를 할당한다.  
사용되지 않는 페이지의 낭비를 방지하기 위한 방식이다.


### 힙 영역

segregated free list 기반의 힙 관리자를 사용한다. slab 할당자로 구현하고 싶었으나, slab은 커널 내부에서 사용되는 객체들의 크기를 미리 알아야만 효율적으로 동작한다. 메모리 관리자는 커널 개발 초기에 구현되어 당시에는 이러한 정보가 없었으므로, segregated free list 기반으로 구현했다.

#### 초기화

[`heap_init()`](https://github.com/zerone015/kfs/blob/master/src/kernel/kmalloc.c#L48)은 물리 메모리 초기화 후 남은 공간에서 시작한다.

초기화 시, 그리고 힙 확장 시마다 커널 페이지 크기인 4MB 단위 블록 끝에 더미 청크 두 개를 추가한다.  
페이지 끝에 위치한 마지막 청크 해제 시 병합 가능 여부를 확인하려면 다다음 청크의 `PREV_INUSE` 비트를 읽어야 하는데, 더미 청크가 없으면 이 접근이 페이지 경계를 넘어 의도치 않은 페이지 폴트를 유발한다.

#### 청크 구조

청크 구조는 ptmalloc2를 참고하여 설계했다.

```c
struct malloc_chunk {
    size_t prev_size;
    size_t size;
    struct list_head list_head;
};
```

`size` 필드의 하위 2비트는 비트 플래그로 활용된다.

| 비트 | 설명 |
|------|------|
| `PREV_INUSE` | 이전 청크 사용 중 여부. 해제 시 병합 판단에 사용 |
| `IS_HEAD` | 가상 블록의 시작 여부. 가상 공간 반납 가능 여부 판단 |

`prev_size`는 구조체 정의상 현재 청크에 속해 있지만, 실제로는 이전 청크의 사용자 데이터 영역 마지막 4바이트를 재활용한다. 이전 청크가 해제될 때 그 공간에 자신의 크기를 써넣고, 현재 청크는 그 값을 `prev_size`로 읽는 구조다.

따라서 할당된 청크는 `size` 필드와 사용자 데이터로만 구성되며, `prev_size`와 `list_head`는 공간을 차지하지 않는다. 해제된 청크는 사용자 데이터 영역을 재활용해 `prev_size`와 `list_head`를 저장한다. 고정 메타데이터 크기를 줄여 힙 메모리를 효율적으로 사용하기 위한 방식이다.

#### 관리 방식

크기별로 분리된 free list 배열을 유지한다.

| 인덱스 | 관리 크기 범위 |
|---------|----------------|
| 0 | 16B ~ 28B |
| 1 | 32B ~ 60B |
| 2 | 64B ~ 124B |
| … | 이후 동일한 규칙으로 확장 |

#### [할당](https://github.com/zerone015/kfs/blob/master/src/kernel/kmalloc.c#L146)

1. 구간 탐색: 요청 크기를 담을 수 있는 최소 구간의 다음 구간부터 탐색 시작. (예: 36B 요청 → index 2부터)
2. 청크 탐색: 각 구간의 리스트 head만 확인한다. 순회하지 않으므로 사실상 스택처럼 동작한다.
3. 청크 분할: 청크가 요청보다 크면 남은 공간을 분할해 적절한 구간의 free list에 삽입한다.
4. 가상 공간 요청: 적합한 청크가 없으면 새 가상 주소 블록을 할당해 힙을 확장한다.

#### [해제](https://github.com/zerone015/kfs/blob/master/src/kernel/kmalloc.c#L168)

1. 인접 청크 병합: 앞뒤가 free 상태라면 더 큰 청크로 병합한다.
2. 리스트 삽입: 병합된 청크를 크기에 맞는 free list에 삽입한다.
3. 가상 블록 반납: 현재 청크가 `IS_HEAD`라면 블록 단위로 가상 주소 공간을 해제한다.

#### 성능

| 연산 | 복잡도 |
|------|--------|
| 할당 | O(log N) |
| 해제 | O(1) |
| 내부 단편화 | 없음 |
| 외부 단편화 | 존재 (최소 구간이 아닌 다음 구간에서 탐색, 각 리스트를 순회하지 않기 때문) |


### 2. 시스템 콜 인터페이스

내부 인터럽트(`int 0x80`) 방식으로 구현했다.  
사용자-커널 인터페이스는 Linux와 최대한 동일하게 설계했다.  
시스템 콜의 동작과 의미는 Linux man page 및 POSIX 문서를 참고했으며, 가능한 범위 내에서 해당 규격을 따른다.

```
int 0x80 (유저 공간)
└── syscall_handler()       # syscall.asm - 레지스터 저장, 커널 스택 전환
    └── syscall_dispatch()  # syscall.c   - 시스템 콜 번호로 핸들러 분기
        └── sys_####()      # 각 시스템 콜 구현
```

#### 1) [fork](https://github.com/zerone015/kfs/blob/master/src/kernel/syscall.c#L421)

부모 프로세스를 복제하여 자식 프로세스를 생성한다.  
매핑된 물리 페이지들은 COW 방식으로 공유되며, 실제 복사는 쓰기 접근 시 페이지 폴트 핸들러에서 이루어진다.

```
sys_fork()
├── process_clone()
│   ├── alloc_pid()
│   ├── child_stack_setup()     # 자식 커널 스택 구성
│   ├── vblocks_clone()         # 할당 가능한 가상 블록 복제
│   ├── mapped_vblocks_clone()  # 할당된 가상 블록 복제
│   ├── pages_set_cow()         # COW 플래그 설정 및 refcount 증가
│   └── pgdir_clone()           # 페이지 디렉터리 및 테이블 복제
├── add_child_to_parent()
├── process_register()
├── add_to_pgroup()
└── ready_queue_enqueue()
```

`child_stack_setup()`은 부모의 레지스터 문맥을 기반으로 자식의 커널 스택을 구성한다.  
`eax`만 0으로 설정해 fork 반환값을 표현하고, 최초 실행 시 트램폴린 코드로 점프하도록 return address를 스택에 배치한다.

자식이 처음 스케줄링되면 커널 스택은 다음과 같이 구성되어 있다.

```
커널 스택 (자식, 최초 실행 시점)
┌─────────────────────────────┐ ← esp0 (커널 스택 최상단)
│  ss                         │ ╮
│  esp                        │ │
│  eflags                     │ │ iretd로 복원되는 유저 문맥 (부모로부터 복사)
│  cs                         │ │
│  eip                        │ ╯
│  eax = 0                    │ ╮
│  ecx                        │ │ 트램폴린에서 pop으로 복원
│  edx                        │ ╯
│  &fork_child_trampoline     │ ← return address
│  ebx                        │ ╮
│  esi                        │ │ 스케줄러에서 pop으로 복원 (부모로부터 복사)
│  edi                        │ │
│  ebp                        │ ╯
└─────────────────────────────┘ ← child->esp
```

스케줄러가 `child->esp`를 로드하고 ebx/esi/edi/ebp를 pop 후 ret하면 `fork_child_trampoline`으로 진입한다.  
트램폴린에서 edx/ecx/eax를 pop하고 iretd를 실행하면 유저 모드로 복귀한다.

COW 설계 결정

페이지 테이블까지 COW로 공유하는 방식도 있으나, 본 프로젝트는 페이지 테이블은 즉시 복사하고 물리 페이지만 공유하는 방식을 택했다.

- COW 없이 전체 복사: 4MB 단위 메모리 전체 복사
- 채택 방식: 페이지 테이블 4KB만 복사
- 페이지 테이블도 COW 공유: PDE 하나(4B)만 복사

가장 큰 비용인 4MB 복사만 피하면 대부분의 이득이 확보된다. 4KB를 4B로 줄여 얻는 추가 이득은 상대적으로 매우 작아, 구현 복잡도가 올라가는 두 번째 방식은 선택하지 않았다.

부모의 시그널 핸들러는 자식에게 복사되며, pending 시그널은 상속되지 않는다.

#### 2) [_exit](https://github.com/zerone015/kfs/blob/master/src/kernel/syscall.c#L458)

프로세스를 즉시 종료하고 자원을 정리한다.

```
sys_exit()
├── resources_cleanup()     # 가상 주소 공간 해제, 프로세스 그룹 정리
├── reparent_children()     # 자식 프로세스 부모를 init으로 재설정
├── defunct_mark()          # 프로세스 상태를 zombie로 변경
├── exit_notify()           # 부모에게 SIGCHLD 전송 또는 wake up
└── yield()                 # 스케줄러에서 제거 후 다음 프로세스로 전환
```

PID, 페이지 디렉터리, 커널 스택, `task_struct`는 즉시 해제되지 않는다.  
종료 중에도 커널 코드가 실행 중이므로 스택과 페이지 디렉터리는 즉시 해제할 수 없고,  
PID와 종료 상태는 부모가 `waitpid`로 회수할 때까지 유지해야 하기 때문이다.

부모가 SIGCHLD를 무시하는 경우, 부모는 종료 상태를 필요로 하지 않으므로 해당 프로세스의 부모를 init으로 재설정하여 init이 잔여 자원을 회수하도록 한다.

#### 3) [waitpid](https://github.com/zerone015/kfs/blob/master/src/kernel/syscall.c#L441)

자식 프로세스의 종료를 감지한다. 지원되는 옵션은 `WNOHANG`이며, 중지/재개 감지는 미구현이다.

```
sys_waitpid()
├── wait_for_pid()     pid > 0
├── wait_for_pgid()    pid == 0  (현재 프로세스 그룹)
├── wait_for_pgid()    pid < -1  (-pid를 PGID로)
└── wait_for_any()     pid == -1

# 위 네 함수는 모두 동일한 흐름으로 수렴한다
├── (좀비 자식 있을 시) child_reap()
└── (없을 시) wait_sleep()
    ├── unmasked_signal_pending() 검사  # 차단 전
    ├── sleep()
    ├── signal_pending() 재확인         # 깨어난 후
    └── child_reap()
```

`child_reap()`에서는 종료 상태를 저장할 주소가 유저 공간인지, 실제 매핑된 주소인지 검증한다.  
커널 주소 공간 덮어쓰기나 잘못된 주소로 인한 페이지 폴트를 방지하기 위해서다.

`wait_sleep()`은 차단 전후 두 번 시그널을 확인한다. 시그널이 있으면 즉시 중단 후 처리하며, 처리 후 시스템 콜이 자동으로 재시작된다.

#### 4) [kill](https://github.com/zerone015/kfs/blob/master/src/kernel/syscall.c#L488)

지정된 프로세스 또는 프로세스 그룹에 시그널을 전송한다.

```
sys_kill()
├── (pid > 0)  kill_one()
└── (pid <= 0) kill_many()
    ├── (pid == -1) kill_to_all()    # 전체 태스크 리스트 순회
    └── (pid <= 0)  kill_to_group()  # 프로세스 그룹 내 탐색
```

커널은 PID/PGID 기반 해시 테이블과 전체 태스크 리스트를 함께 유지한다.  
시그널은 대상 프로세스의 pending 목록에 추가되며, 실제 처리는 유저 모드로 복귀할 때 수행된다.  
대상이 차단 상태라면 즉시 깨운다.

#### 5) [signal](https://github.com/zerone015/kfs/blob/master/src/kernel/syscall.c#L474)

특정 시그널에 대해 유저 정의 핸들러를 등록하고 기존 핸들러를 반환한다.

```
sys_signal()
├── signal_is_valid()
├── signal_is_catchable()   # SIGKILL, SIGSTOP 제외
├── sighandler_lookup()     # 기존 핸들러 저장
└── sighandler_register()
```

각 프로세스는 `task_struct` 내부에 시그널 번호를 인덱스로 사용하는 핸들러 배열을 가진다.

지원되는 시그널:

```c
#define SIGINT   2    #define SIGKILL  9    #define SIGCHLD  17
#define SIGILL   4    #define SIGSEGV  11   #define SIGCONT  18
#define SIGABRT  6    #define SIGTERM  15   #define SIGSTOP  19
#define SIGFPE   8
```

#### 6) [sigreturn](https://github.com/zerone015/kfs/blob/master/src/kernel/syscall.asm#L58)

시그널 핸들러 실행 전 저장해둔 유저 문맥을 복구하여 원래 실행 흐름으로 돌아간다.

```
sys_sigreturn()
├── EIP 검증                    # 커널 주소 가리킬 경우 SIGSEGV로 종료
├── current_signal 초기화       # 재진입 방지
├── EFLAGS 복원                 # 유저 플래그만 복원, 특권 비트는 커널 값 유지
├── 유저 문맥 복원
└── iretd
```

EIP 검증과 EFLAGS 특권 비트 보호는 조작된 시그널 프레임을 통한 SROP 공격을 방어하기 위한 조치다.

#### 시그널 처리 흐름

시그널 전송 시 pending 목록에 추가되고, 실제 처리는 유저 모드 복귀 직전에 수행된다.

[`isr_signal_return()`](https://github.com/zerone015/kfs/blob/master/src/kernel/signal.asm#L8)과 [`syscall_signal_return()`](https://github.com/zerone015/kfs/blob/master/src/kernel/signal.asm#L35)은 각각 인터럽트/예외와 시스템 콜 복귀 경로다. [`unmasked_signal_pending()`](https://github.com/zerone015/kfs/blob/master/src/kernel/signal.c#L145C5-L145C28)이 참이면 [`do_signal()`](https://github.com/zerone015/kfs/blob/master/src/kernel/signal.c#L113)로 수렴한다.

```
isr_signal_return()       # 인터럽트/예외 복귀 경로
syscall_signal_return()   # 시스템 콜 복귀 경로
    └── unmasked_signal_pending()
        ├── false → iretd
        └── true  → do_signal()
                     ├── SIG_DFL → do_default_signal()
                     └── 핸들러 등록 → do_caught_signal()
                                        └── sigframe_setup()
```

[`do_caught_signal()`](https://github.com/zerone015/kfs/blob/master/src/kernel/signal.c#L98)은 유저 스택에 시그널 프레임을 구성한 뒤 커널 스택에 저장된 EIP를 핸들러 주소로, ESP를 프레임 위치로 덮어쓴다. `iretd`로 복귀하면 자연스럽게 핸들러가 실행되며, 핸들러가 반환하면 트램폴린 코드가 `sigreturn`을 호출해 원래 문맥으로 복구된다.

`do_caught_signal()` 실행 후 커널 스택과 유저 스택의 전체 그림은 다음과 같다.

```
커널 스택
┌─────────────────────────────┐
│  ss                         │
│  esp      → sf              │ ← 원래 ucontext->esp에서 sf로 덮어씀
│  eflags                     │
│  cs                         │
│  eip      → 핸들러 주소      │ ← 원래 eip에서 핸들러 주소로 덮어씀
└─────────────────────────────┘
유저 스택
┌─────────────────────────────┐ ← 원래 ucontext->esp
│  eflags                     │
│  eip                        │ ← sigreturn 복귀 후 실행 재개 주소
│  eax, ecx, edx, ebx         │
│  esi, edi, ebp, esp ────────┼──→ 원래 ucontext->esp
│  trampoline[8]              │ ← mov eax, 119 / int 0x80
│  arg    = sig               │ ← 핸들러 첫 번째 인자
│  ret    = &trampoline       │ ← 핸들러 return address
├─────────────────────────────┤ ← sf = ESP (핸들러 진입 시점)
│  핸들러 스택 프레임          │
└─────────────────────────────┘
```

핸들러 실행 중 동일한 시그널이 도착하면 재진입 문제가 발생할 수 있다. `current_signal`이 설정되어 있는 동안 즉시 처리되지 않고 pending 상태로 지연되며, 핸들러 종료 후 다시 처리된다.


### 3. 인터럽트

#### 예외

커널 모드에서 발생한 복구 불가능한 예외는 패닉으로 이어지고, 유저 모드에서 발생한 예외는 해당 프로세스에 시그널을 전송하여 종료시킨다.

##### [페이지 폴트](https://github.com/zerone015/kfs/blob/master/src/kernel/isr.c#L115)

```
page_fault_handle(fault_addr, error_code)
├── user_space(fault_addr)
│   ├── pgtab_allocated() && is_rdwr_cow()    → COW 처리
│   ├── pgtab_allocated() && entry_reserved() → 물리 페이지 매핑
│   └── 그 외                                 → SIGSEGV
└── kernel_space(fault_addr)
    ├── pf_is_usermode()                      → SIGSEGV
    ├── entry_reserved()                      → 물리 페이지 매핑
    └── 그 외                                 → 패닉
```

##### [패닉](https://github.com/zerone015/kfs/blob/master/src/kernel/panic.asm)

<img src="assets/panic.png" alt="커널 패닉 화면" width="650"/>

패닉 진입점은 호출 맥락에 따라 두 가지로 나뉜다. `do_panic`은 일반 커널 코드에서, `isr_panic`은 ISR/예외 핸들러에서 호출된다. 두 맥락은 스택 레이아웃이 다르므로 진입점을 분리했으며, 각각 스택에서 레지스터 값을 수집해 통일된 [`panic_info`](https://github.com/zerone015/kfs/blob/master/include/panic.h#L12C1-L15C3) 프레임을 구성한 뒤 [`panic()`](https://github.com/zerone015/kfs/blob/master/src/kernel/panic.c#L59)을 호출한다.

```
do_panic / isr_panic
└── panic_info 프레임 구성 (레지스터 수집)
    └── panic()
        ├── tty_clear()
        ├── hex_dump()        # EIP, EFLAGS, 범용 레지스터, 스택 덤프 출력
        ├── panic_msg_print()
        └── system_halt()     # 레지스터 초기화, cli, hlt 루프
```

#### 하드웨어 인터럽트

##### [PIT 타이머 인터럽트 (IRQ0)](http://github.com/zerone015/kfs/blob/master/src/kernel/isr.c#L145)

PIT는 1ms 주기로 인터럽트를 발생시키며 스케줄링 타이머로 사용된다.

```
IRQ0 → pit_handler → pit_handle()
                     ├── pic_send_eoi()
                     ├── time_slice_remaining--
                     └── time_slice_remaining == 0 → schedule()
```

타이머를 10ms가 아닌 1ms로 설정한 이유는 자발적 선점 시 공정성 때문이다.  
10ms 타이머라면 자발적 선점 시점에서 다음 인터럽트까지 남은 시간이 다음 태스크의 타임 슬라이스가 되므로, 최악의 경우 거의 0ms에 가까운 슬라이스를 받을 수 있다.  
1ms 타이머라면 이 오차가 1ms 미만으로 줄어든다.

##### [키보드 인터럽트 (IRQ1)](https://github.com/zerone015/kfs/blob/master/src/kernel/isr.c#L156)

US QWERTY 레이아웃을 지원한다. PS/2 컨트롤러의 데이터 포트에서 스캔코드를 PIO 방식으로 읽어오며, USB 키보드는 USB Legacy 지원을 통해 PS/2처럼 에뮬레이션된다.

```
IRQ1 → keyboard_handler → keyboard_handle()
                          ├── 스캔코드 읽기 (PS/2 데이터 포트)
                          ├── 문자 변환 (key_map / shift_key_map)
                          └── VGA 텍스트 모드 버퍼에 출력
```


### 4. 스케줄링

[스케줄러](https://github.com/zerone015/kfs/blob/master/include/sched.h#L28)는 라운드 로빈 방식의 선점형 멀티태스킹 구조다. 타임 슬라이스는 10ms이며, 유저 모드에서는 선점이 허용되고 커널 모드에서는 비활성화된다.

```
IRQ0 → pit_handler → schedule() → switch_to()
blocking syscall   → yield()    → schedule() → switch_to()
```

#### 문맥 교환

1. 자동 저장 (CPU): SS, ESP, EFLAGS, CS, EIP를 커널 스택에 push
2. 휘발성 레지스터 저장 (컴파일러): cdecl 규약에 따라 EAX, ECX, EDX를 push
3. 비휘발성 레지스터 저장 (커널): [`switch_to()`](https://github.com/zerone015/kfs/blob/master/src/kernel/sched.asm#L9)에서 EBX, ESI, EDI, EBP를 push
4. 현재 프로세스 정보 저장: 현재 ESP를 `task_struct`에 저장, `current`를 다음 태스크로 교체
5. 다음 프로세스 문맥 복원: 다음 태스크의 ESP 로드, CR3이 다를 경우에만 교체(불필요한 TLB flush 방지), TSS.esp0 갱신 후 비휘발성 레지스터 pop
6. 유저 모드 복귀: 함수 epilogue에서 휘발성 레지스터 자동 복구, IRET로 EIP·ESP·EFLAGS 복원


## 테스트 및 검증

구현된 기능의 안정성을 확인하기 위해 별도의 테스트 코드를 작성 및 검증했다.  
관련 테스트 코드는 [`test/`](https://github.com/zerone015/kfs/tree/master/test) 디렉토리에서 확인할 수 있다.  
시스템 콜, 시그널, 스케줄링 관련 테스트는 유저 모드 환경에서 실행되도록 구성했다.

#### [물리 메모리](https://github.com/zerone015/kfs/blob/master/test/pmm_test.c#L141) (`test_pmm`)
- 오더별 단일 크기 반복 할당/해제 후 free 페이지 수가 초기 상태와 일치하는지 검증
- 여러 오더를 섞어 할당/해제 후 일관성 검증
- 할당된 물리 주소가 BIOS 메모리 맵의 사용 가능 영역 내에 있는지 검증

#### [가상 주소 공간](https://github.com/zerone015/kfs/blob/master/test/vmm_test.c#L156) (`test_vmm`)
- 단일 4MB 블록 할당 후 범위/정렬 확인, 페이지 폴트 발생 여부 확인, 해제 후 병합 검증
- 가용 공간 전체 소진 후 추가 할당 실패 확인, 전량 해제 후 병합 검증
- 크기를 늘려가며 할당, 짝수/홀수 인덱스 순으로 해제 후 전체 크기 블록 재할당 가능 여부로 병합 완전성 검증

#### [힙](https://github.com/zerone015/kfs/blob/master/test/hmm_test.c#L33) (`test_malloc`)
- 8B~8192B 다양한 크기로 128MB 한도 내 최대 개수 할당
- 패턴 기록 후 검증으로 데이터 무결성 확인
- 짝수/홀수 인덱스 순으로 해제하여 비연속 해제 후 재할당 가능 여부 검증
- 3회 반복하여 재사용 안정성 확인

#### [fork](https://github.com/zerone015/kfs/blob/master/test/fork_test.c#L33) (`test_fork`)
- 1000회 반복 fork 후 자식의 PID, COW 동작(magic number 독립성) 검증
- 자식이 추가 fork를 수행하는 중첩 fork 시나리오 검증

#### [wait / _exit](https://github.com/zerone015/kfs/blob/master/test/wait_exit_test.c#L32) (`test_wait_exit`)
- 1000만 회 반복 fork/exit/wait 스트레스 테스트
- 종료 상태(`WIFEXITED`, `WEXITSTATUS`) 및 반환 PID 정확성 검증

#### [시그널](https://github.com/zerone015/kfs/blob/master/test/signal_test.c) (`test_signal`)
- SIGKILL 전송 후 `wait`로 종료 상태 검증 (`WIFSIGNALED`, `WTERMSIG`)
- 유저 공간에 시그널 핸들러를 직접 매핑하여 핸들러 실행 여부 확인

#### [커널 패닉](https://github.com/zerone015/kfs/blob/master/test/panic_test.asm)
- 레지스터를 의도적으로 설정한 뒤 `do_panic` 호출, 출력값과 일치 여부 확인
- 잘못된 주소(`0xDEADBEFF`)로 점프하여 예외 발생 시 패닉 처리 검증