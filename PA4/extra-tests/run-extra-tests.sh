#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

pass=0
fail=0

run_expect_success() {
  name=$1
  shift
  if ./mysemant "$@" >/tmp/pa4-extra.out 2>/tmp/pa4-extra.err; then
    echo "PASS good $name"
    pass=$((pass + 1))
  else
    echo "FAIL good $name"
    cat /tmp/pa4-extra.err
    fail=$((fail + 1))
  fi
}

run_expect_failure() {
  name=$1
  shift
  if ./mysemant "$@" >/tmp/pa4-extra.out 2>/tmp/pa4-extra.err; then
    echo "FAIL bad $name"
    fail=$((fail + 1))
  else
    if grep -q "Compilation halted due to static semantic errors." /tmp/pa4-extra.err; then
      echo "PASS bad $name"
      pass=$((pass + 1))
    else
      echo "FAIL bad $name"
      cat /tmp/pa4-extra.err
      fail=$((fail + 1))
    fi
  fi
}

run_expect_success good.cl good.cl
run_expect_failure bad.cl bad.cl

for file in extra-tests/good/01_basic_dispatch.cl \
            extra-tests/good/02_self_type_and_static.cl \
            extra-tests/good/03_lub_case_if.cl \
            extra-tests/good/04_scope_shadowing.cl \
            extra-tests/good/05_basic_ops.cl; do
  run_expect_success "$file" "$file"
done

run_expect_success "extra-tests/good/06_multi_file_*.cl" \
  extra-tests/good/06_multi_file_lib.cl \
  extra-tests/good/06_multi_file_main.cl

for file in extra-tests/bad/*.cl; do
  run_expect_failure "$file" "$file"
done

for file in /var/tmp/cool/examples/arith.cl \
            /var/tmp/cool/examples/book_list.cl \
            /var/tmp/cool/examples/cells.cl \
            /var/tmp/cool/examples/complex.cl \
            /var/tmp/cool/examples/graph.cl \
            /var/tmp/cool/examples/hairyscary.cl \
            /var/tmp/cool/examples/hello_world.cl \
            /var/tmp/cool/examples/io.cl \
            /var/tmp/cool/examples/lam.cl \
            /var/tmp/cool/examples/life.cl \
            /var/tmp/cool/examples/list.cl \
            /var/tmp/cool/examples/new_complex.cl \
            /var/tmp/cool/examples/palindrome.cl \
            /var/tmp/cool/examples/primes.cl \
            /var/tmp/cool/examples/sort_list.cl; do
  run_expect_success "official $(basename "$file")" "$file"
done

run_expect_success "official atoi with test" \
  /var/tmp/cool/examples/atoi.cl \
  /var/tmp/cool/examples/atoi_test.cl

echo "extra-tests: $pass passed, $fail failed"
test "$fail" -eq 0
