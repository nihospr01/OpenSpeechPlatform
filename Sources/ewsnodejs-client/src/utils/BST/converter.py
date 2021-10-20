#!/usr/bin/env python3

import csv
import requests
import json

headNodes = []
nodeNChildren = []
nodeN = []


createTableURL = "http://localhost:5000/api/db/table/create/bst"
URL = "http://localhost:5000/api/db/bst"
headers = {"Content-Type": "application/json"}

# create bst table in db
createRes = requests.post(createTableURL)
assert createRes.status_code != 500



# parsing rootNodes.csv for head-nodes entry
with open('rootNodes.csv') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if line_count == 0:
            line_count += 1
        else:
            headNodes.append(int(row[0]))
            line_count += 1

    # key-value dictionary
    headNodesDict = {"key": "head-nodes", "value": headNodes}

    # post request for head nodes
    body = json.dumps(headNodesDict)
    bodyDump = json.dumps(body)
    headRes = requests.post(URL, headers=headers, data=bodyDump)
    print(headRes.status_code)
    print(headRes.text)
    print(body)
    assert headRes.status_code == 200
    assert headRes.text == ""


# parsing treeGraph.csv for node-n-children entries
with open('treeGraph.csv') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    currParent = 0
    children = [[0 for x in range(0)] for y in range(76)] 

    for row in csv_reader:
        if line_count == 0:
            line_count += 1
        else:
            currParent = row[1]

            index = int(currParent)
            if index == -1:
                index = 0
            data = int(row[0])
            children[index].append(data)

    
    for (i, item) in enumerate(children, start=1):
        # key-value dictionary
        key = "node-{}-children".format(i-1)
        nodeChildDict = {key: item }

        # post request for children
        body = json.dumps(nodeChildDict)
        childrenRes = requests.post(URL, headers=headers, data=body)
        assert childrenRes.status_code == 200
        assert childrenRes.text == ""



# parsing treeNodes.csv for node-n entries
with open('treeNodes.csv') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    column_names = []
    column_ctr = 0
    for row in csv_reader:
        if line_count == 0:
            for item in row:
                column_names.append(item)
            line_count += 1
        else:
            # populate json string for each node
            nodeDict = {}
            for item in row:
                x = { column_names[column_ctr]: item}
                nodeDict.update(x)
                column_ctr += 1
            nodeN.append(nodeDict)
            column_ctr = 0
            line_count += 1
            body = json.dumps(nodeDict)
            
            # post request for RTMHA params of each node
            nodeRes = requests.post(URL, headers=headers, data=body)
            assert rnodeReses.status_code == 200
            assert nodeRes.text == ""

            
