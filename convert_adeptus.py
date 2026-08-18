#!/usr/bin/env python3

import argparse
import sqlite3
import os
import re
import socket
from pathlib import Path
from typing import Any
from datetime import datetime

PROP_NAMES = ["Category", "Severity", "Priority", "Repeat", "Status", "Solution"]
INLINE_LINK = r"(^|\s)#(\d+)($|\s)"
STATION_NAME = socket.gethostname()
CONVERSION_DATE = datetime.now().astimezone().isoformat(timespec="seconds")
MEMO_TYPE = "issue"
NEW_MEMOS: dict[int, dict[str, Any]] = {}

def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(
    description="Convert Adeptus (*.bugs) database to Procyon (*.enot)."
  )
  parser.add_argument("in_path", type=Path, help="Input Adeptus database (*.bugs)")
  parser.add_argument("out_path", type=Path, help="Output Procyon database (*.enot)")
  parser.add_argument("folder_id", type=int, help="Target folder id")
  parser.add_argument("--verbose", "-v", action="store_true", help="Print addition info")
  return parser.parse_args()

if __name__ == "__main__":
  args = parse_args()
  
  print("===============================================")
  print(f"Source: {args.in_path}")
  if not os.path.isfile(args.in_path):
    raise Exception("Input file not found")
  adeptus_conn = sqlite3.connect(args.in_path)
  adeptus = adeptus_conn.cursor()
  if args.verbose:
    print("  Tables in source file:")
    adeptus.execute("SELECT name FROM sqlite_schema WHERE type = 'table'")
    for name in adeptus.fetchall():
      print(f"    {name[0]}")

  print("===============================================")
  print(f"Target: {args.out_path}")
  if not os.path.isfile(args.out_path):
    raise Exception("Input file not found")
  enot_conn = sqlite3.connect(args.out_path)
  enot = enot_conn.cursor()
  if args.verbose:
    print("  Tables in target file:")
    enot.execute("SELECT name FROM sqlite_schema WHERE type = 'table'")
    for name in enot.fetchall():
      print(f"    {name[0]}")

  print("===============================================")
  print(f"Loading dictionaries...")
  all_prop_values: dict[str, dict[int, str]] = {}
  for prop_name in PROP_NAMES:
    if args.verbose:
      print(f"  {prop_name}:")
    table_name = "Repeatability" if prop_name == "Repeat" else prop_name
    adeptus.execute(f"SELECT Id, Title FROM {table_name}")
    prop_values: dict[int, str] = {}
    for r in adeptus.fetchall():
      id, title = int(r[0]), str(r[1])
      prop_values[id] = title
      if args.verbose:
        print(f"    {id} = {title}")
    all_prop_values[prop_name] = prop_values

  print("===============================================")
  enot.execute("SELECT MAX(Id) FROM Memo")
  new_memo_id = enot.fetchone()[0]
  if new_memo_id is None: new_memo_id = 0
  print(f"Max memo id: {new_memo_id}")

  print("===============================================")
  print("Reading issues...")
  adeptus.execute("SELECT Id, Summary, Extra, Created, Updated," +
    "Category, Severity, Priority, Repeat, Status, Solution FROM Issue")
  for r in adeptus.fetchall():
    issue_id, summary, extra, created, updated = r[0], r[1], r[2], r[3], r[4]

    new_memo_id += 1

    memo_props: dict[str, str] = {}
    for prop_idx, prop_name in enumerate(PROP_NAMES):
      if (value_id := int(r[prop_idx + 5])) > 0:
        value = all_prop_values[prop_name].get(value_id)
        memo_props[prop_name] = value if value else str(value_id)

    memo_opts: dict[str, str] = {
      "adeptus": f"{issue_id}|{args.out_path.name}|{CONVERSION_DATE}"
    }

    if args.verbose:
      print("------------------------------------------------")
      print(f"#{issue_id} {summary}")
      print(f"Id: {issue_id} --> {new_memo_id}")
      print(f"Properties: {memo_props}")
      print(f"Options: {memo_opts}")

    NEW_MEMOS[issue_id] = {
      "id": new_memo_id,
      "title": summary,
      "data": extra,
      "created": created,
      "updated": updated,
      "props": memo_props,
      "opts": memo_opts,
    }


  print("===============================================")
  print("Correcting inline links...")
  for issue_id, memo in NEW_MEMOS.items():

    def replace_inline_link(match: re.Match):
      linked_issue_id = int(match.group(2))
      linked_memo_id = NEW_MEMOS.get(linked_issue_id, {}).get("id")
      if not linked_memo_id:
        print(f"WARN: memo not found for issue {issue_id}, skip")
        return str(linked_issue_id)
      print(f"In issue {issue_id} (memo {memo['id']}): replace {linked_issue_id} --> {linked_memo_id}")
      return str(linked_memo_id)

    old_data: str = memo["data"]
    new_data: str = re.sub(INLINE_LINK, replace_inline_link, old_data, flags=re.MULTILINE)

  print("===============================================")
  print("Writing memos...")
  for issue_id, memo in NEW_MEMOS.items():
    enot.execute(f"SELECT MemoId FROM MemoOptions WHERE Name = 'adeptus' AND Value LIKE '{issue_id}|%'")
    row = enot.fetchone()
    print(f"FFFFFFF {row}")
    memo_id = row[0] if row else None
    if memo_id:
      print(f"Issue {issue_id} is already converted into memo {memo_id}, skip")
      memo["id"] = memo_id
    else:
      memo_id = memo["id"]
      if args.verbose:
        print(f"Inserting issue {issue_id} as memo {memo_id}...")
      enot.execute(f"INSERT INTO Memo " +
        "(Id, Parent, Title, Type, Data, Updated, Created, Station)" +
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        (memo_id, args.folder_id, memo["title"], MEMO_TYPE, 
          memo["data"], memo["updated"], memo["created"], STATION_NAME))
      for name, value in memo["props"].items():
        if args.verbose:
          print(f"Inserting property {name}...")
        enot.execute("INSERT INTO MemoProps (MemoId, Name, Value) VALUES (?, ?, ?)",
          (memo["id"], name, value))
      for name, value in memo["opts"].items():
        if args.verbose:
          print(f"Inserting option {name}...")
        enot.execute("INSERT INTO MemoOptions (MemoId, Name, Value) VALUES (?, ?, ?)",
          (memo["id"], name, value))
      enot_conn.commit()

  print("===============================================")
  print("Writing links...")
  adeptus.execute("SELECT Id1, Id2, Created FROM Relations")
  for r in adeptus.fetchall():
    issue_id_1, issue_id_2, created = int(r[0]), int(r[1]), r[2]
    memo_id_1 = NEW_MEMOS.get(issue_id_1, {}).get("id")
    if not memo_id_1:
      print(f"WARN: memo not found for issue {issue_id_1}, skip")
      continue
    memo_id_2 = NEW_MEMOS.get(issue_id_2, {}).get("id")
    if not memo_id_2:
      print(f"WARN: memo not found for issue {issue_id_2}, skip")
      continue
    enot.execute("SELECT * FROM MemoLinks WHERE (Id1=? AND Id2=?) OR (Id1=? AND Id2=?)",
      (memo_id_1, memo_id_2, memo_id_2, memo_id_1))
    if enot.fetchone():
      print(f"Link {memo_id_1}-{memo_id_2} already exists, skip")
      continue
    if args.verbose:
      print(f"Inserting link {memo_id_1}-{memo_id_2}")
    enot.execute("INSERT INTO MemoLinks (Id1, Id2, Created) VALUES (?, ?, ?)",
      (memo_id_1, memo_id_2, created))
    enot_conn.commit()

  adeptus_conn.close()
  enot_conn.close()
