"""Failure-oriented checks for the design bank's acceptance gate (no audio tests)."""
import ast
from copy import deepcopy
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parent))
import build_bank as bank


class BankContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=bank.recipes()
        cls.schemas={"Hamlo"+n:bank.read_schema("Hamlo"+n)[0] for n in bank.PLUGINS}

    def module(self,data,name):
        for row in data:
            for m in row["chain"]+[m for z in row["regions"] for m in z["chain"]]:
                if m["plugin"]=="Hamlo"+name: return row,m
        self.fail("Module missing: "+name)

    def test_authored_bank_passes_current_native_contract(self):
        result=bank.validate(self.data,self.schemas)
        self.assertEqual(result["recipes"],250)
        self.assertEqual(result["unique_processing"],250)
        self.assertEqual(set(result["module_instances"]),set(self.schemas))
        self.assertTrue(all(r["calibration"]["status"]=="not_run" for r in self.data))

    def test_no_shadowed_authored_helpers(self):
        tree=ast.parse((Path(__file__).parent/"recipes.py").read_text(encoding="utf-8"))
        names=[n.name for n in tree.body if isinstance(n,(ast.FunctionDef,ast.ClassDef))]
        self.assertEqual(len(names),len(set(names)),"A second helper silently shadows the first")

    def test_numeric_reader_accepts_cpp_units_without_executing_code(self):
        self.assertEqual(bank.number("1.f/64"),1/64)
        self.assertEqual(bank.number("-.25f"),-.25)
        for text in ("float(variable)","__import__('os')","other.value"):
            with self.subTest(text=text),self.assertRaises(ValueError): bank.number(text)

    def test_count_and_identity_rejections(self):
        for defect in ("missing_recipe","duplicate_id","duplicate_name","duplicate_processing"):
            data=deepcopy(self.data)
            if defect=="missing_recipe": data.pop()
            elif defect=="duplicate_id": data[1]["id"]=data[0]["id"]
            elif defect=="duplicate_name": data[1]["name"]=data[0]["name"]
            else:
                for key in ("eq","chain","regions","output_mode"): data[1][key]=deepcopy(data[0][key])
            with self.subTest(defect=defect),self.assertRaises(ValueError): bank.validate(data,self.schemas)

    def test_unknown_out_of_range_nonfinite_and_fractional_choice(self):
        for key,value in (("typoThreshold",-20),("thresholdDb",-61),("ratio",float("nan")),("model",1.5)):
            data=deepcopy(self.data);_,m=self.module(data,"Comp");m["parameters"][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError): bank.validate(data,self.schemas)

    def test_geometry_and_capacity_rejections(self):
        for defect in ("eq_range","chain_capacity","region_order","empty_region"):
            data=deepcopy(self.data)
            if defect=="eq_range": data[0]["eq"][0]["hz"]=24000
            elif defect=="chain_capacity": data[0]["chain"]*=4
            elif defect=="region_order": data[0]["regions"]=[dict(low_hz=500,high_hz=100,chain=[data[0]["chain"][0]])]
            else: data[0]["regions"]=[dict(low_hz=100,high_hz=500,chain=[])]
            with self.subTest(defect=defect),self.assertRaises(ValueError): bank.validate(data,self.schemas)

    def test_wet_send_and_engine_guards(self):
        for name,key,value in (("Echo","scoreRouting",0),("Echo","scoreMix",.5),
                ("Verb","keepDry",1),("Verb","engine",0),("Comp","model",0),
                ("Zastava","v2_external",1),("Shirota","v2Guard",0),("Udarnik","protect",0)):
            data=deepcopy(self.data);_,m=self.module(data,name);m["parameters"][key]=value
            with self.subTest(name=name,key=key),self.assertRaises(ValueError): bank.validate(data,self.schemas)

    def test_diagnostic_noise_and_freeze_guards(self):
        for name,key,value in (("Mag","hiss",.1),("Mag","delta",1),("Echo","scoreFreeze",1),
                ("Fader","solo",1),("RedWall","monitor",2)):
            data=deepcopy(self.data);_,m=self.module(data,name);m["parameters"][key]=value
            with self.subTest(name=name,key=key),self.assertRaises(ValueError): bank.validate(data,self.schemas)

    def test_explicit_scene_cannot_disagree_with_lane_switches(self):
        for defect in ("dimension","action","empty_lane"):
            data=deepcopy(self.data);_,m=self.module(data,"Revolution")
            if defect=="dimension": m["scene"]["lanes"][0].pop()
            elif defect=="action": m["scene"]["lanes"][1][0]=10
            else: m["parameters"]["motionOn"]=1;m["scene"]["lanes"][2]=[0]*16
            with self.subTest(defect=defect),self.assertRaises(ValueError): bank.validate(data,self.schemas)

    def test_match_capture_cannot_pretend_to_be_a_measured_state(self):
        data=deepcopy(self.data)
        row=next(r for r in data if "required_action" in r["calibration"])
        del row["calibration"]["required_action"]
        with self.assertRaisesRegex(ValueError,"Match capture"): bank.validate(data,self.schemas)

    def test_scene_uses_three_valid_native_pattern_masks(self):
        for defect in ("seven_masks","negative_lock","invalid_allow","fractional_lock"):
            data=deepcopy(self.data);_,m=self.module(data,"Revolution")
            scene=m["scene"]
            if defect=="seven_masks": scene["locks"]=[65535]*7
            elif defect=="negative_lock": scene["locks"][0]=-1
            elif defect=="invalid_allow": scene["allow_masks"][0]=64
            else: scene["locks"][0]=1.5
            with self.subTest(defect=defect),self.assertRaisesRegex(ValueError,"Scene"):
                bank.validate(data,self.schemas)

    def test_master_requires_a_final_ceiling_stage(self):
        data=deepcopy(self.data);row=next(r for r in data if r["category"]=="Master")
        row["chain"].reverse()
        with self.assertRaisesRegex(ValueError,"Final master"): bank.validate(data,self.schemas)

    def test_wet_return_rejects_dry_native_routing(self):
        data=deepcopy(self.data);row=next(r for r in data if r["output_mode"]=="wet_only")
        row["chain"][0]["amount"]=.5
        with self.assertRaisesRegex(ValueError,"Wet-only"): bank.validate(data,self.schemas)


if __name__=="__main__":
    unittest.main(verbosity=2)
