#!/usr/bin/env python3
"""
spec-conformance 스킬(축소판): main/*.c가 참조하는 CH422G_EXIO_*, *_GPIO_NUM 매크로가
실제로 include/pin_config.h에 정의되어 있는지 자동 대조한다.

이걸 CI(또는 Claude Code의 verify.ps1)에서 코드 수정 후마다 돌리면,
"내가 지어낸 매크로 이름"과 "실제 프로젝트에 정의된 이름"의 불일치(Case A 유형)를
컴파일 전에, 그리고 사람이 파일을 눈으로 대조하기 전에 잡아낼 수 있다.

사용법: python3 spec_conformance.py <pin_config.h> <main .c 파일들...>
"""
import re
import sys

def extract_defined_macros(header_path):
    defined = set()
    with open(header_path, encoding="utf-8") as f:
        content = f.read()
    for m in re.finditer(r"#define\s+([A-Za-z_][A-Za-z0-9_]*)", content):
        defined.add(m.group(1))
    return defined

def extract_referenced_identifiers(src_path):
    with open(src_path, encoding="utf-8") as f:
        content = f.read()
    # 이 프로젝트 컨벤션에 해당하는 식별자만 대상으로 함 (오탐 줄이기 위해 접두사 한정)
    pattern = re.compile(r"\b(CH422G_EXIO_[A-Z0-9_]+|[A-Z0-9_]+_GPIO_NUM|I2C_MASTER_[A-Z0-9_]+)\b")
    return set(pattern.findall(content))

def main():
    if len(sys.argv) < 3:
        print("usage: spec_conformance.py <pin_config.h> <src.c> [more.c ...]")
        sys.exit(2)

    header_path = sys.argv[1]
    src_paths = sys.argv[2:]

    defined = extract_defined_macros(header_path)
    violations = []

    for src in src_paths:
        used = extract_referenced_identifiers(src)
        undefined = sorted(used - defined)
        for name in undefined:
            violations.append((src, name))

    if violations:
        print("❌ spec-conformance 실패 — pin_config.h에 없는 식별자를 참조 중:\n")
        for src, name in violations:
            print(f"  {src}: {name}")
        print(f"\n총 {len(violations)}건. pin_config.h를 먼저 확인하거나, "
              f"의도한 신규 매크로라면 pin_config.h에 먼저 추가하세요 (AGENTS.md 2번 항목).")
        sys.exit(1)
    else:
        print(f"✅ spec-conformance 통과 — {len(src_paths)}개 파일, "
              f"pin_config.h({len(defined)}개 매크로) 기준 미정의 참조 없음.")
        sys.exit(0)

if __name__ == "__main__":
    main()
