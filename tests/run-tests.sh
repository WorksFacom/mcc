#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
compiler="$project_dir/mcc"
test_dir=$(mktemp -d)
compiler_only=false

if [[ ${1:-} == "--compiler-only" ]]; then
    compiler_only=true
fi

cleanup() {
    rm -rf "$test_dir"
}
trap cleanup EXIT

positive_examples=(
    arrayprint base basicarray bubble busca fatorial fibonacci hanoi
    maximo_divisor primos soma teste_impressao
)
positive_regressions=(
    nested_calls frame_scopes unary_short_circuit forward_call string_escape
    large_frame char_return
)

run_positive() {
    local source_file=$1
    local expected_file=$2
    local name=$3
    local asm_file="$test_dir/$name.s"
    local executable="$test_dir/$name"

    #gera o assembly usando o compilador Micro C
    (cd "$test_dir" && "$compiler" --gen-asm "$source_file" -o "$asm_file" >"$name.compile.log" 2>&1)
    if $compiler_only; then
        return
    fi

    #monta e liga o programa gerado
    gcc -Wl,-z,noexecstack "$asm_file" -o "$executable"

    #compara a saída real com o resultado esperado
    "$executable" >"$test_dir/$name.actual"
    diff -u "$expected_file" "$test_dir/$name.actual"
}

for name in "${positive_examples[@]}"; do
    run_positive "$project_dir/exemplos/$name.mcc" \
                 "$project_dir/tests/expected/$name.txt" "$name"
done

for name in "${positive_regressions[@]}"; do
    run_positive "$project_dir/tests/regression/$name.mcc" \
                 "$project_dir/tests/expected/$name.txt" "$name"
done

if ! $compiler_only; then
    negative_sources=(
        "$project_dir/exemplos/erro_lexico.mcc"
        "$project_dir/exemplos/erro_sintatico.mcc"
        "$project_dir/exemplos/erro_semantico.mcc"
        "$project_dir/tests/lexer/unterminated_comment.mcc"
        "$project_dir/tests/lexer/identifier_too_long.mcc"
        "$project_dir/tests/parser/array_without_size.mcc"
        "$project_dir/tests/semantic/missing_return.mcc"
        "$project_dir/tests/semantic/missing_main.mcc"
        "$project_dir/tests/semantic/string_overflow.mcc"
    )
    negative_messages=(
        "Erro Lexico" "Erro Sintatico" "Erro Semantico"
        "Erro Lexico" "Erro Lexico" "Erro Sintatico"
        "pode terminar sem retornar" "declarar a funcao main"
        "String e muito grande"
    )

    for i in "${!negative_sources[@]}"; do
        log="$test_dir/negative-$i.log"
        if (cd "$test_dir" && "$compiler" "${negative_sources[$i]}" >"$log" 2>&1); then
            echo "Teste negativo aceito indevidamente: ${negative_sources[$i]}" >&2
            exit 1
        fi
        grep -q "${negative_messages[$i]}" "$log"
    done
fi

echo "Todos os testes passaram."
