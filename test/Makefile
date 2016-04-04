.PHONY: test
.PHONY: generate

test: test_complete_first_5_blocks

generate: generate_complete_first_5_blocks

test_complete_first_5_blocks:
	./scripts/print_all --quiet --utxo-period 1 --end 5 2>/dev/null | diff --label GIVEN --label EXPECTED --unified fixtures/print_all.utxo-period=1.end=5 -

generate_complete_first_5_blocks:
	./scripts/print_all --utxo-period 1 --end 5 > fixtures/print_all.utxo-period=1.end=5
