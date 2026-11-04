"""Check source profile boundaries, not a hardware or complete patch validation."""
import json
from pathlib import Path
p=json.loads((Path(__file__).resolve().parents[1]/'Profiles/SH01A_SOURCE_V1.json').read_text(encoding='utf-8'))
assert p['profileId']=='roland.sh-01a.source.v1' and p['status']=='SOURCE_VERIFIED_NOT_BOUND'
assert p['outputPort'] is None and p['channel1Based'] is None and p['baseReference'] is None
assert not p['machineOutputConnected'] and not p['programChange']['enabled']
assert not p['bindings'] and not p['programReferences'] and not p['resetDefaults']
cc=[r['cc'] for r in p['parameters']]
assert len(cc)==42 and len(set(cc))==42 and 84 not in cc
assert next(r for r in p['parameters'] if r['cc']==74)['name']=='VCF CUTOFF'
assert next(r for r in p['parameters'] if r['cc']==71)['name']=='VCF RESONANCE'
assert all(r['valueEncoding'] is None and r['defaultValue'] is None for r in p['parameters'])
assert {r['cc'] for r in p['parameters'] if not r['transmitted']}=={11,64,65}
assert not p['systemExclusive']['recognized'] and not p['systemExclusive']['requests']
assert not p['notes']['engineGeneratesMusicalNotes']
print('PASS SH-01A source profile: 42 explicit CC, no invented bindings/defaults/PC/port; hardware NOT verified')

# The new working assumption is separate from the historical source inventory.
import re
root=Path(__file__).resolve().parents[1]
working=json.loads((root/'Profiles/SH01A_PATCH36_PROVISIONAL_V1.json').read_text(encoding='utf-8'))
assert working['status']=='PROVISIONAL_USER_AUTHORIZED_NOT_HARDWARE_QUALIFIED'
assert working['hardwareQualified'] is False and working['writeMidiSupported'] is False
assert len(working['cc'])==36 and len(set(working['cc']))==36
tx={r['cc'] for r in p['parameters'] if r['transmitted']}
assert tx-set(working['cc'])==set(working['excludedFromSourceTx'])=={1,76,77}
header=(root/'Shared/XoneSh01aProfile.hpp').read_text(encoding='utf-8')
match=re.search(r'Sh01aProvisionalCc36\{\{([^}]+)\}\}',header)
assert match and [int(v) for v in match.group(1).split(',')]==working['cc']
print('PASS provisional SH36 table matches C++ and documented TX identities; recall membership remains an assumption')
