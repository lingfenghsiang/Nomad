import re
import copy
import csv
import os
import json5 as json
import pandas as pd
import argparse

this_file_dir = os.path.abspath(os.path.dirname(__file__))

class LogParseConfig:
    def __init__(self, config_file: str) -> None:
        with open(config_file, 'r') as f:
            data = json.load(f)
            if "section_start_with" in data:
                self.start_ = data["section_start_with"]
            else:
                exit("incomplete config")
            
            if "section_end_with" in data:
                self.end_ = data["section_end_with"]
            else:
                exit("incomplete config")
            
            if "group_by" in data:
                self.group_by_ = data["group_by"]
            else:
                exit("incomplete config")
            
            if "data_columns" in data:
                self.data_columns_ = data["data_columns"]
            else:
                exit("incomplete config")

            if "order_by" in data:
                self.order_by_ = data["order_by"]
            else:
                exit("incomplete config")                              
            

class LogParser:
    def __init__(self, config: LogParseConfig):
        self.data_list_ = []
        self.attributes_ = []
        # 0: not in the middle of a transaction, 1 processing a transaction
        self.stage_flag_ = 0
        self.seperator_start_ = re.compile(config.start_)
        self.seperator_end_ = re.compile(config.end_)
        self.pair_match_expression_ = re.compile(r"\[(.*?)\]\ *:\ *\[(.*?)\]")


    def scanFile(self, file_name, tmp_dir = None):
        f = open(file_name)
        new_item = 0
        kv_dict = {}
        while(1):                
            line = f.readline()
            if(not line):
                break
            seperate_start_status = re.search(self.seperator_start_, line)
            seperate_end_status = re.search(self.seperator_end_, line)
            # if we have detected a new item
            if(seperate_start_status):
                self.stage_flag_ = 1
            # if we come to the end of this item, dump this item
            if(seperate_end_status):
                self.stage_flag_ = 0
                if(kv_dict):
                    self.data_list_.append(copy.deepcopy(kv_dict))
                kv_dict.clear()
            if(self.stage_flag_ == 0):
                continue
            
            
            obj = re.findall(self.pair_match_expression_, line)
            if(obj):
                for kv_pair in obj:
                    kv_dict[kv_pair[0]] = kv_pair[1]
                
        f.close()
        if(tmp_dir):
            tmp_log_path = os.path.join(tmp_dir, "dumped_data.csv")
        else:
            tmp_log_path =this_file_dir+'/dumped_data.csv'

        with open(tmp_log_path, 'w') as f:
            headers = dict.keys(self.data_list_[0])
            f_csv = csv.DictWriter(f, headers)
            f_csv.writeheader()
            f_csv.writerows(self.data_list_)

class dataFrameMerger:
    def __init__(self) -> None:
        pass
    def mergeDf(self, df, config:LogParseConfig, output_file_name):

        if config.group_by_:
            grouped = df.groupby(config.group_by_)
            keys = grouped.groups.keys()
            tmp_list = []
            first_time = 0
            for i in keys:
                print(type(i))
                tmp = grouped.get_group(i)
                tmp = tmp.drop(columns = config.group_by_)
                if not first_time:
                    first_time = 1
                else:
                    tmp = tmp.drop(columns = config.order_by_)
                if type(i) is tuple:
                    prefix = '_'.join(str(x) for x in i)
                    tmp = tmp.rename(columns=lambda x: prefix + '_' + x if x not in config.order_by_ else x)
                else:
                    tmp = tmp.rename(columns=lambda x: str(i) + '_' + x if x not in config.order_by_ else x)
                tmp_list.append(tmp.reset_index(drop=True))
            final_df = pd.concat(tmp_list, axis=1)
        else:
            final_df = df
        if config.order_by_:
            final_df = final_df.sort_values(by = config.order_by_)
        final_df = final_df.reset_index(drop = True)
        # c = pd.concat(tmplist, axis=1)
        # c.insert(0, col_name, first_col.reset_index(drop=True))
        final_df.to_csv(output_file_name)

class tabGenerator:
    def __init__(self, config: LogParseConfig, output_name: str):
        self.config_ = config
        self.output_ = output_name
    def parser(self, df, out_dir = None):
        tmp = dataFrameMerger()
        if (out_dir):
            out_path = os.path.join(out_dir, self.output_)
        else:
            out_path = self.output_
        tmp.mergeDf(df, self.config_, out_path)


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='format logs.')
    parser.add_argument('-l', '--log_path', type=str, default= "test.log",
                        help='path to log')
    parser.add_argument('-c', '--config_path', type=str, default="config.json",
                        help='path to log')
    parser.add_argument('-o', '--output_path', type=str, default="result.csv",
                        help='path to output csv file')
    args = parser.parse_args()

    config = LogParseConfig(args.config_path)

    formatted_log = LogParser(config)
    formatted_log.scanFile(args.log_path, tmp_dir = this_file_dir)

    df = pd.read_csv(os.path.join(this_file_dir, 'dumped_data.csv'))

    worker = tabGenerator(config, args.output_path)
    worker.parser(df)
    os.remove(os.path.join(this_file_dir, 'dumped_data.csv'))
    