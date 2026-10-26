"""Verify the imported candidate and preserved documentary inventories."""
import csv
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
manifest = json.loads((root/'docs/reprise/MANIFEST_L01_11.json').read_text())
for item in manifest['files']:
    assert hashlib.sha256((root/item['path']).read_bytes()).hexdigest() == item['sha256'], item['path']
assert len(list((root/'Core').glob('*.hpp'))) == 12
assert len(list((root/'NativeTests').glob('test_*.cpp'))) == 14
for name,count in [('MATRICE_RECONCILIEE_L01.csv',81),('HANDLERS_RECLASSES_L01.csv',382)]:
    with (root/'docs/reprise'/name).open(encoding='utf-8-sig',newline='') as f:
        assert len(list(csv.DictReader(f))) == count, name
print('PASS candidate manifest: 12 headers, 14 suites; 81 REQ and 382 handler rows')
