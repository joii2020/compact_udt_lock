MOLC := moleculec
MOLC_VERSION := 0.7.0
MOLC2 := moleculec-c2

PROTOCOL_SCHEMA := blockchain.mol
PROTOCOL_HEADER := blockchain.h
PROTOCOL_VERSION := d75e4c56ffa40e17fd2fe477da3f98c5578edcd1
PROTOCOL_URL := https://raw.githubusercontent.com/nervosnetwork/ckb/${PROTOCOL_VERSION}/util/types/schemas/blockchain.mol

XUDT_SCHEMA := xudt_rce.mol
XUDT_HEADER := xudt_rce_mol.h
XUDT_HEADER2 := xudt_rce_mol2.h
XUDT_OUTPUT := xudt_rce_mol2.json
XUDT_VERSION := 9a4b25b1b7a9d42b46cfd050acfcd55100ed5089
XUDT_URL := https://raw.githubusercontent.com/nervosnetwork/ckb-production-scripts/${XUDT_VERSION}/c/xudt_rce.mol

COMPACT_UDT_SCHEMA := compact_udt.mol
COMPACT_UDT_HEADER := compact_udt_mol.h
COMPACT_UDT_HEADER2 := compact_udt_mol2.h
COMPACT_UDT_OUTPUT := compact_udt_mol2.json

all:
	make clean_mol
	make ${PROTOCOL_SCHEMA}
	make ${PROTOCOL_HEADER}
	make ${XUDT_SCHEMA}
	make ${XUDT_HEADER}
	make ${XUDT_HEADER2}
	make ${COMPACT_UDT_HEADER}
	make ${COMPACT_UDT_HEADER2}

${PROTOCOL_SCHEMA}:
	curl -L -o $@ ${PROTOCOL_URL}

${PROTOCOL_HEADER}: ${PROTOCOL_SCHEMA}
	${MOLC} --language c --schema-file $< > $@

${XUDT_SCHEMA}:
	curl -L -o $@ ${XUDT_URL}

${XUDT_HEADER}: ${XUDT_SCHEMA}
	${MOLC} --language c --schema-file $< > $@

${XUDT_HEADER2}: ${XUDT_SCHEMA}
	${MOLC} --language - --schema-file $< --format json > ${XUDT_OUTPUT}
	${MOLC2} --input ${XUDT_OUTPUT} | clang-format -style=Google > $@

${COMPACT_UDT_HEADER}: ${COMPACT_UDT_SCHEMA}
	${MOLC} --language c --schema-file $< > $@

${COMPACT_UDT_HEADER2}: ${COMPACT_UDT_SCHEMA}
	${MOLC} --language - --schema-file $< --format json > ${COMPACT_UDT_OUTPUT}
	${MOLC2} --input ${COMPACT_UDT_OUTPUT} | clang-format -style=Google > $@

clean_mol:
	rm -f ${PROTOCOL_SCHEMA}
	rm -f ${PROTOCOL_HEADER}
	rm -f ${XUDT_SCHEMA}
	rm -f ${XUDT_UDT_HEADER}
	rm -f ${XUDT_UDT_HEADER2}
	rm -f ${XUDT_UDT_OUTPUT}
	rm -f ${COMPACT_UDT_HEADER}
	rm -f ${COMPACT_UDT_HEADER2}
	rm -f ${COMPACT_UDT_OUTPUT}
