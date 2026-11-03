from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import inspect_items


class ItemCatalogueTests(unittest.TestCase):
    def fixture(self):
        data = bytearray(200)
        struct.pack_into('<HII',data,0,30,126,200)
        struct.pack_into('<H',data,126+0x12,1)
        struct.pack_into('<3H',data,146,26,1000,2000)
        # One wall cabinet and one Table A record with a misleading family.
        struct.pack_into('<HH4HH4HH4H',data,152,
                         0,2,100,200,100,0,1,0,1,0x3205,0x2064,1,0,0x5004,0,0)
        return data

    def test_wall_and_table_a_remain_distinct(self):
        result = inspect_items.catalogue(self.fixture())
        self.assertEqual(result['table_a_count'],1)
        self.assertEqual(len(result['wall_panels']),1)
        wall = result['wall_panels'][0]
        self.assertEqual((wall['type'],wall['contents'],wall['a'],wall['b']),
                         ('0x3205','0x2064',[1100,2200],[1100,2000]))
        self.assertEqual((wall['kind'],wall['amount']),(2,100))

    def test_malformed_data_rejected(self):
        for data in (b'',self.fixture()[:170]):
            with self.assertRaises(ValueError):
                inspect_items.catalogue(data)
        data = self.fixture()
        struct.pack_into('<H',data,168,7) # Invalid first vertex index.
        with self.assertRaises(ValueError):
            inspect_items.catalogue(data)


if __name__ == '__main__':
    unittest.main()
