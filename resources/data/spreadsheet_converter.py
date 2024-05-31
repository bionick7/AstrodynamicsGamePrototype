import yaml
import csv
import difflib

from copy import deepcopy
from typing import Any
from dictdiffer import diff

yaml_t = list[dict[str, Any]]
csv_t = list[list[Any]]

keys = ["id", "name", "description", "type", "mass"]
construction_stats = [
    "ground_connection",         # Required to build and run resource extraction
    "thermal_control",           # Required to build stuff makeing tonns of heat
    "industrial_admin",          # Required to build 'complex' stuff in the early game
    "industrial_storage",        # Required to build big stuff and ships
    "industrial_manufacturing",  # Required to build anything
    "industrial_dock",           # Required to build ships and armor
    "cryogenics_facility",       # Required to handle anything invloving hydrogen
    "clean_room",                # Required to build anything related to semiconductors/optics
    "bio_manufacturing",         # Required to build life support stuff
    "arms_manufacturing",        # Required to build boarding stuff and invasion equipment
    "precision_manufacturing",   # Gates more 'high-tech' stuff together with clean-room
    "nuclear_enrichment",        # Required to build reactors and nuclear-powered ships
    "military_training",         # Required to make troops for boarding
]

stat_list = [
    "power",
    "initiative",
    "kinetic_hp",
    "energy_hp",
    "crew",
    "kinetic_offense",
    "ordnance_offense",
    "boarding_offense",
    "kinetic_defense",
    "ordnance_defense",
    "boarding_defense",
    *construction_stats
]

resource_list = [
    "water",
    "hydrogen",
    "oxygen",
    "rock",
    "iron_ore",
    "steel",
    "aluminium_ore",
    "aluminium",
    "food",
    "biomass",
    "waste",
    "co2",
    "carbon",
    "polymers",
    "electronics",
]

def load_yaml_data() -> yaml_t:
    with open("resources/data/ship_modules.yaml", "rt") as f:
        yaml_loader = yaml.loader.SafeLoader(f)
        data = yaml_t(yaml_loader.get_data()["shipmodules"])
        for row in data:
            if len(row.get("description", "")) > 0 and row["description"][-1] == "\n":
                row["description"] = row["description"][:-1]
    return data


def save_yaml_data(data: yaml_t):
    with open("resources/data/ship_modules.yaml", "wt") as f:
        yaml.safe_dump({"shipmodules": data}, f, indent=2, sort_keys=False, )


def load_csv_data(category: str) -> csv_t:
    with open(f"resources/data/spreadsheets/ship_modules-{category}.csv", "rt") as f:
        csv_reader = csv.reader(f)
        data: csv_t = list(csv_reader)
        for row in data:
            for i in range(len(row)):
                if isinstance(row[i], str) and row[i].isdigit():
                    row[i] = int(row[i])
    return data


def save_csv_data(data: csv_t, category: str):
    with open(f"resources/data/spreadsheets/ship_modules-{category}.csv", "wt") as f:
        csv_writer = csv.writer(f, lineterminator="\n")
        csv_writer.writerows(data)


def export_csv_general(data: yaml_t) -> csv_t:
    csv_data = [[module.get(key, "") for key in keys] for module in data]
    for i in range(len(csv_data)):
        requs = data[i].get("construction_reqirements", {"industrial_manufacturing": 1})
        for construction_stat in construction_stats:
            if construction_stat in requs:
                csv_data[i].append(requs[construction_stat])
            else:
                csv_data[i].append(" ")
        csv_data[i].append(data[i].get("construction_time", 20))
    csv_data.insert(0, [*keys, "power", *construction_stats, "construction_time"])
    return csv_data


def export_csv_construction(data: yaml_t) -> csv_t:
    csv_data = [[module["id"]] for module in data]
    for i in range(len(data)):
        construction_resources = data[i].get("construction_resources", {})
        for resource in resource_list:
            if resource in construction_resources:
                csv_data[i].append(construction_resources[resource])
            else:
                csv_data[i].append(" ")
    csv_data.insert(0, ["id", *resource_list])
    return csv_data


def export_csv_production(data: yaml_t) -> csv_t:
    csv_data = [[module["id"]] for module in data]
    for i in range(len(data)):
        production = data[i].get("produce", {})
        for resource in resource_list:
            if resource in production:
                csv_data[i].append(production[resource])
            else:
                csv_data[i].append(" ")
    csv_data.insert(0, ["id", *resource_list])
    return csv_data


def export_csv_stats(data: yaml_t) -> csv_t:
    csv_data = [[module["id"]] for module in data]
    for i in range(len(csv_data)):
        stat_add = data[i].get("add", {})
        for stat in stat_list:
            if stat in stat_add:
                csv_data[i].append(stat_add[stat])
            else:
                csv_data[i].append(" ")
    csv_data.insert(0, ["id", *stat_list])
    return csv_data


def fuse_genral_csv(data_csv_raw: csv_t, data_yaml: yaml_t) -> yaml_t:
    data = deepcopy(data_yaml)
    csv_header = data_csv_raw[0]
    data_csv = data_csv_raw[1:]

    for csv_row in data_csv:
        id = csv_row[csv_header.index("id")]
        yaml_row_result_list = [x for x in data if x["id"] == id]
        if len(yaml_row_result_list) == 0:
            yaml_row = { "id": id }
            data.append(yaml_row)
        else:
            yaml_row = yaml_row_result_list[0]
            
        assert csv_row[0] == yaml_row["id"]

        for i, k in enumerate(keys):
            yaml_row[k] = csv_row[i]
        if "construction_time" in yaml_row or csv_row[csv_header.index("construction_time")] != 20:
            construction_time_entry = csv_row[csv_header.index("construction_time")]
            if isinstance(construction_time_entry, str) and "construction_time" in yaml_row:
                del yaml_row["construction_time"]
            else:
                yaml_row["construction_time"] = construction_time_entry
        
        construction_requirements = csv_row[csv_header.index(construction_stats[0]) : csv_header.index(construction_stats[-1]) + 1]
        assert len(construction_requirements) == len(construction_stats)
        construction_requirements = [0 if isinstance(x, str) else x for x in construction_requirements]

        construction_requirements_are_default = sum(construction_requirements) == 1 and construction_requirements[4] == 1

        if "construction_reqirements" not in yaml_row:
            if construction_requirements_are_default:
                continue
            else:
                yaml_row["construction_reqirements"] = {}

        for i, construction_stat in enumerate(construction_stats):
            if construction_requirements[i] != 0:
                yaml_row["construction_reqirements"][construction_stat] = construction_requirements[i]

    return data


def _retrieve_int(x: Any) -> int:
    if isinstance(x, str):
        if len(x) == 0: return 0
        if x.isdigit() or (x[0] == "-" and x[1:].isdigit()):
            return int(x)
        return 0
    elif isinstance(x, (int, float)):
        return int(x)
    return 0


def fuse_auxiliary_csv(data_csv_raw: csv_t, data_yaml: yaml_t, key: str, total_list: list[str]) -> yaml_t:
    data = deepcopy(data_yaml)
    csv_header = data_csv_raw[0]
    data_csv = data_csv_raw[1:]

    for csv_row in data_csv:
        id = csv_row[csv_header.index("id")]
        yaml_row_result_list = [x for x in data if x["id"] == id]
        if len(yaml_row_result_list) == 0:
            yaml_row = { "id": id }
            data.append(yaml_row)
        else:
            yaml_row = yaml_row_result_list[0]
            
        data_row = list(map(_retrieve_int, csv_row[1:]))
        assert len(data_row) == len(total_list)

        if key not in yaml_row:
            if sum(data_row) == 0:
                continue
            else:
                yaml_row[key] = {}

        for i, construction_resource in enumerate(total_list):
            if data_row[i] != 0:
                yaml_row[key][construction_resource] = data_row[i]

    return data


def generate_module_spreadsheets():
    data_yaml = load_yaml_data()
    data_csv_general = export_csv_general(data_yaml)
    data_csv_construction = export_csv_construction(data_yaml)
    data_csv_production = export_csv_production(data_yaml)
    data_csv_stats = export_csv_stats(data_yaml)
    save_csv_data(data_csv_general, "general")
    save_csv_data(data_csv_construction, "construction")
    save_csv_data(data_csv_production, "production")
    save_csv_data(data_csv_stats, "stats")


def fuse_module_spreadsheets():
    data_yaml = load_yaml_data()
    for row in data_yaml:
        row["construction_reqirements"] = {}
        row["construction_resources"] = {}
        row["produce"] = {}
        row["add"] = {}
    data_csv_general = load_csv_data("general")
    data_csv_construction = load_csv_data("construction")
    data_csv_production = load_csv_data("production")
    data_csv_stats = load_csv_data("stats")
    data_res = fuse_genral_csv(data_csv_general, data_yaml)
    data_res = fuse_auxiliary_csv(data_csv_construction, data_res, "construction_resources", resource_list)
    data_res = fuse_auxiliary_csv(data_csv_production, data_res, "produce", resource_list)
    data_res = fuse_auxiliary_csv(data_csv_stats, data_res, "add", stat_list)
    save_yaml_data(data_res)


def test_generation_and_fusion():
    data_yaml = load_yaml_data()
    data_csv_general = export_csv_general(data_yaml)
    data_csv_construction = export_csv_construction(data_yaml)
    data_csv_production = export_csv_production(data_yaml)
    data_csv_stats = export_csv_stats(data_yaml)
    data_res = fuse_genral_csv(data_csv_general, data_yaml)
    data_res = fuse_auxiliary_csv(data_csv_construction, data_res, "construction_resources", resource_list)
    data_res = fuse_auxiliary_csv(data_csv_production, data_res, "produce", resource_list)
    data_res = fuse_auxiliary_csv(data_csv_stats, data_res, "add", stat_list)
    diffrence = list(diff(data_yaml, data_res))
    print(diffrence)
    assert len(diffrence) == 0

if __name__ == "__main__":
    #generate_module_spreadsheets()
    fuse_module_spreadsheets()
    #test_generation_and_fusion()
