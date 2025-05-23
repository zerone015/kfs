#ifndef _EXT2_H
#define _EXT2_H

#include <stdint.h>

#define EXT2_SUPER_BLOCK_LBA    2
#define EXT2_MAGIC              0xEF53

enum ext2_state {
    EXT2_VALID_FS = 1,              // Unmounted cleanly
    EXT2_ERROR_FS = 2,              // Errors detected
};

enum ext2_errors {
    EXT2_ERRORS_CONTINUE = 1,       // continue as if nothing happened
    EXT2_ERRORS_RO = 2,             // remount read-only
    EXT2_ERRORS_PANIC = 3,          // cause a kernel panic
};

enum ext2_rev_level {
    EXT2_GOOD_OLD_REV = 0,          // Revision 0
    EXT2_DYNAMIC_REV = 1,           // Revision 1 with variable inode sizes, extended attributes, etc.
};

struct ext2_super_block {
    uint32_t s_inodes_count;        // 총 아이노드 수
    uint32_t s_blocks_count;        // 총 블록 수
    uint32_t s_r_blocks_count;      // 슈퍼 유저가 사용하도록 예약 된 블록 수
    uint32_t s_free_blocks_count;   // 사용 가능한 블록 수
    uint32_t s_free_inodes_count;   // 사용 가능한 아이노드 수
    uint32_t s_first_data_block;    // 첫 데이터 블록 (1024 block size면 1, 4096이면 0. 즉, 슈퍼 블록이 포함된 블록 ID)
    uint32_t s_log_block_size;      // 블록 크기 = 1024 << s_log_block_size
    uint32_t s_log_frag_size;       // 조각 크기 (사용되지 않음)
    uint32_t s_blocks_per_group;    // 그룹당 블록 수 (s_first_data_block과 함께 블록 그룹의 경계를 결정하는데 사용됨)
    uint32_t s_frags_per_group;     // 그룹당 조각 수 (각 블록 그룹의 block 비트맵 크기를 결정하는데 사용됨)
    uint32_t s_inodes_per_group;    // 그룹당 아이노드 수 (각 블록 그룹의 inode 비트맵을 결정하는데 사용됨)
    uint32_t s_mtime;               // 마지막 마운트 된 시간
    uint32_t s_wtime;               // 마지막 쓰기 시간
    uint16_t s_mnt_count;           // 마운트 횟수
    uint16_t s_max_mnt_count;       // 최대 마운트 횟수
    uint16_t s_magic;               // 파일 시스템을 ext2로 식별하는 마법값 (0xEF53)
    uint16_t s_state;               // 파일 시스템 상태
    uint16_t s_errors;              // 오류 처리 방식
    uint16_t s_minor_rev_level;     // 마이너 버전
    uint32_t s_lastcheck;           // 마지막 체크 시간
    uint32_t s_checkinterval;       // 체크 주기
    uint32_t s_creator_os;          // 생성 OS
    uint32_t s_rev_level;           // 버전
    uint16_t s_def_resuid;          // 예약 블록용 기본 UID
    uint16_t s_def_resgid;          // 예약 블록용 기본 GID
    uint32_t s_first_ino;           // 첫 번째 비예약 아이노드 번호
    uint16_t s_inode_size;          // 아이노드 구조체 크기
    uint16_t s_block_group_nr;      // 이 슈퍼블록이 속한 블록 그룹
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];            // 파일 시스템 UUID
    char     s_volume_name[16];     // 볼륨 이름
    char     s_last_mounted[64];    // 마지막 마운트 경로
    uint32_t s_algo_bitmap;         // 압축 알고리즘에서 사용됨
    uint8_t  s_prealloc_blocks;
    uint8_t  s_prealloc_dir_blocks;
    uint16_t s_padding1;
    uint8_t  s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    uint32_t s_hash_seed[4];
    uint8_t  s_def_hash_version;
    uint8_t  s_reserved_char_pad;
    uint16_t s_reserved_word_pad;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;
    uint32_t s_reserved[190];
} __attribute__((packed));

struct ext2_group_desc {
    uint32_t bg_block_bitmap;       // 블록 비트맵의 블록 번호
    uint32_t bg_inode_bitmap;       // 아이노드 비트맵의 블록 번호
    uint32_t bg_inode_table;        // 아이노드 테이블의 시작 블록 번호
    uint16_t bg_free_blocks_count;  // 이 그룹에 있는 비어 있는 블록 수
    uint16_t bg_free_inodes_count;  // 이 그룹에 있는 비어 있는 아이노드 수
    uint16_t bg_used_dirs_count;    // 할당된 디렉터리 수
    uint16_t bg_pad;                // 패딩
    uint32_t bg_reserved[3];        // 예약된 공간
} __attribute__((packed));

struct ext2_inode {
    uint16_t i_mode;        // 파일 유형 및 접근 권한
    uint16_t i_uid;         // 소유자 UID
    uint32_t i_size;        // 파일 크기 (바이트)
    uint32_t i_atime;       // 마지막 접근 시간
    uint32_t i_ctime;       // 생성 시간
    uint32_t i_mtime;       // 수정 시간
    uint32_t i_dtime;       // 삭제 시간
    uint16_t i_gid;         // 소유자 그룹 ID
    uint16_t i_links_count; // 링크 수
    uint32_t i_blocks;      // 파일이 차지하는 블록 수 (512바이트 단위)
    uint32_t i_flags;       // 플래그 (immutable, append-only 등)
    uint32_t i_osd1;        // OS에 종속적인 값 (리눅스에서는 reserved)

    uint32_t i_block[15];   // 직접/간접 블록 포인터
                            // [0..11]  직접 포인터
                            // [12]     단일 간접
                            // [13]     이중 간접
                            // [14]     삼중 간접

    uint32_t i_generation;  // 파일 버전 (NFS 등에서 사용)
    uint32_t i_file_acl;    // ACL 확장 (블록 번호, 주로 0)
    uint32_t i_dir_acl;     // 디렉터리일 경우, 파일 크기 상위 32비트 (ext2에서만)
    uint32_t i_faddr;       // 프래그먼트 주소 (ext2에서는 거의 사용 안 함)

    uint8_t  i_osd2[12];    // OS 종속 필드 (Linux: uid/gid 확장 등)
} __attribute__((packed));

#endif