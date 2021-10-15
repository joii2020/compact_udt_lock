MOLC := moleculec
MOLC_VERSION := 0.7.0
MOLC2 := moleculec-c2

PROTOCOL_HEADER := blockchain.h
PROTOCOL_SCHEMA := blockchain.mol
PROTOCOL_VERSION := d75e4c56ffa40e17fd2fe477da3f98c5578edcd1
PROTOCOL_URL := https://raw.githubusercontent.com/nervosnetwork/ckb/${PROTOCOL_VERSION}/util/types/schemas/blockchain.mol

RCLOCK_HEADER := rc_lock_mol.h
RCLOCK_SCHEMA := rc_lock.mol
RCLOCK_VERSION := bdb0ac346237f25cdcda5f1f09d42bc4036d98b6
RCLOCK_URL := https://raw.githubusercontent.com/nervosnetwork/ckb-miscellaneous-scripts/${RCLOCK_VERSION}/c/rc_lock.mol

COMPACT_UDT_HEADER := compact_udt_mol.h
COMPACT_UDT_SCHEMA := compact_udt.mol
COMPACT_UDT_HEADER2 := compact_udt_mol2.h
COMPACT_UDT_OUTPUT := compact_udt_mol2.json

all:
	make clean_mol
	make ${PROTOCOL_SCHEMA}
	make ${PROTOCOL_HEADER}
	make ${RCLOCK_SCHEMA}
	make ${RCLOCK_HEADER}
	make ${COMPACT_UDT_HEADER}
	make ${COMPACT_UDT_HEADER2}

${PROTOCOL_SCHEMA}:
	curl -L -o $@ ${PROTOCOL_URL}

${PROTOCOL_HEADER}: ${PROTOCOL_SCHEMA}
	${MOLC} --language c --schema-file $< > $@

${RCLOCK_SCHEMA}:
	curl -L -o $@ ${RCLOCK_URL}

${RCLOCK_HEADER}: ${RCLOCK_SCHEMA}
	${MOLC} --language c --schema-file $< > $@

${COMPACT_UDT_HEADER}: ${COMPACT_UDT_SCHEMA}
	${MOLC} --language c --schema-file $< > $@

${COMPACT_UDT_HEADER2}: ${COMPACT_UDT_SCHEMA}
	${MOLC} --language - --schema-file $< --format json > ${COMPACT_UDT_OUTPUT}
	${MOLC2} --input ${COMPACT_UDT_OUTPUT} | clang-format -style=Google > $@

clean_mol:
	rm -f ${PROTOCOL_SCHEMA}
	rm -f ${PROTOCOL_HEADER}
	rm -f ${RCLOCK_SCHEMA}
	rm -f ${RCLOCK_HEADER}
	rm -f ${COMPACT_UDT_HEADER}
	rm -f ${COMPACT_UDT_HEADER2}
	rm -f ${COMPACT_UDT_OUTPUT}
