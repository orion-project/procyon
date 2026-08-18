#!/usr/bin/env python3

import argparse
import sqlite3
import os
import re
import socket
from pathlib import Path
from typing import Any
from datetime import datetime
from functools import partial

PROP_NAMES = ["Category", "Severity", "Priority", "Repeat", "Status", "Solution"]
DICT_ID_OFFSET = 3
INLINE_LINK = r"(^|\s)#(\d+)($|\s)"
STATION_NAME = socket.gethostname()
CONVERSION_DATE = datetime.now().astimezone().isoformat(timespec="seconds")
MEMO_TYPE = "issue"
NEW_MEMOS: dict[int, dict[str, Any]] = {}
COUNT_MEMOS = 0
COUNT_PROPS = 0
COUNT_OPTS = 0
COUNT_LINKS = 0
COUNT_LINKS_INLINE = 0
COUNT_HISTORY = 0
COUNT_COMMENST = 0

def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(
    description="Convert Adeptus (*.bugs) database to Procyon (*.enot)."
  )
  parser.add_argument("in_path", type=Path, help="Input Adeptus database (*.bugs)")
  parser.add_argument("out_path", type=Path, help="Output Procyon database (*.enot)")
  parser.add_argument("folder_id", type=int, help="Target folder id")
  parser.add_argument("-v", "--verbose", action="store_true", help="Print addition info")
  return parser.parse_args()

def replace_inline_link(issue_id: int, memo_id: int, match: re.Match):
  linked_issue_id = int(match.group(2))
  linked_memo_id = NEW_MEMOS.get(linked_issue_id, {}).get("id")
  if not linked_memo_id:
    print(f"WARN: memo not found for issue {issue_id}, skip")
    return str(linked_issue_id)
  if linked_issue_id != linked_memo_id:
    print(f"In issue {issue_id} (memo {memo_id}): replace {linked_issue_id} --> {linked_memo_id}")
    global COUNT_LINKS_INLINE
    COUNT_LINKS_INLINE += 1
  return str(linked_memo_id)

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
    memo["data"] = re.sub(INLINE_LINK, partial(replace_inline_link, issue_id, memo["id"]), memo["data"], flags=re.MULTILINE)

  print("===============================================")
  print("Writing memos...")
  for issue_id, memo in NEW_MEMOS.items():
    enot.execute("SELECT MemoId FROM MemoOptions INNER JOIN Memo on Id = MemoId "
                 f"WHERE Parent={args.folder_id} AND Name = 'adeptus' AND Value LIKE '{issue_id}|{args.out_path.name}|%'")
    row = enot.fetchone()
    memo_id = row[0] if row else None
    if memo_id:
      print(f"SKIP: issue {issue_id} is already converted into memo {memo_id}")
      memo["id"] = memo_id
    else:
      memo_id = memo["id"]
      if args.verbose:
        print(f"NEW: issue {issue_id} as memo {memo_id}")
      enot.execute(f"INSERT INTO Memo " +
        "(Id, Parent, Title, Type, Data, Updated, Created, Station)" +
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        (memo_id, args.folder_id, memo["title"], MEMO_TYPE, 
          memo["data"], memo["updated"], memo["created"], STATION_NAME))
      COUNT_MEMOS += 1
      for name, value in memo["props"].items():
        if args.verbose:
          print(f"NEW: property {name}...")
        enot.execute("INSERT INTO MemoProps (MemoId, Name, Value) VALUES (?, ?, ?)",
          (memo["id"], name, value))
        COUNT_PROPS += 1
      for name, value in memo["opts"].items():
        if args.verbose:
          print(f"NEW: option {name}...")
        enot.execute("INSERT INTO MemoOptions (MemoId, Name, Value) VALUES (?, ?, ?)",
          (memo["id"], name, value))
        COUNT_OPTS += 1
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
      print(f"SKIP: link {memo_id_1}-{memo_id_2} already exists")
      continue
    if args.verbose:
      print(f"NEW: link {memo_id_1}-{memo_id_2}")
    enot.execute("INSERT INTO MemoLinks (Id1, Id2, Created) VALUES (?, ?, ?)",
      (memo_id_1, memo_id_2, created))
    COUNT_LINKS += 1
    enot_conn.commit()

  print("===============================================")
  print("Writing history...")
  for issue_id, memo in NEW_MEMOS.items():
    memo_id = memo["id"]
    has_history = False
    adeptus.execute("SELECT EventNum, EventPart, ChangedParam, NewValue, Moment FROM History " +
      f"WHERE Issue={issue_id} AND ChangedParam >= {DICT_ID_OFFSET} ORDER BY EventNum, EventPart")
    for r in adeptus.fetchall():
      event_num, event_part, dict_id, value_id, moment = int(r[0]), int(r[1]), int(r[2]), int(r[3]), r[4]
      prop_idx = dict_id - DICT_ID_OFFSET
      if prop_idx < 0 or prop_idx >= len(PROP_NAMES):
        print(f"WARN: invalid dict id {dict_id} for issue {issue_id} (EventNum={event_num}, EventPart={event_part}), skip")
        continue
      prop_name = PROP_NAMES[prop_idx]
      prop_value = all_prop_values.get(prop_name, {}).get(value_id)
      if not prop_value: prop_value = str(value_id)
      what_str = "prop:" + prop_name
      enot.execute("SELECT MemoId FROM MemoHistory WHERE MemoId=? AND What=? AND Value=? AND Moment=?",
        (memo_id, what_str, prop_value, moment))
      if enot.fetchone():
        print(f"SKIP: prop history for issue {issue_id} (memo {memo_id}): {prop_name}={prop_value} already exists")
        continue
      if args.verbose:
        print(f"NEW: prop history for issue {issue_id} (memo {memo_id}): {prop_name}={prop_value}")
      enot.execute("INSERT INTO MemoHistory (MemoId, What, Value, Moment, Station) VALUES (?, ?, ?, ?, ?)",
        (memo_id, what_str, prop_value, moment, STATION_NAME))
      has_history = True
      COUNT_HISTORY += 1
    if has_history:
      enot_conn.commit()

  print("===============================================")
  print("Writing comments...")
  for issue_id, memo in NEW_MEMOS.items():
    memo_id = memo["id"]
    has_comments = False
    adeptus.execute("SELECT EventNum, EventPart, Comment, Moment FROM History " +
      f"WHERE Issue={issue_id} AND ChangedParam < 0 ORDER BY EventNum, EventPart")
    for r in adeptus.fetchall():
      event_num, event_part, comment, moment = int(r[0]), int(r[1]), r[2], r[3]
      enot.execute("SELECT Id FROM MemoSheets WHERE MemoId=? AND Created=?", (memo_id, moment))
      if enot.fetchone():
        print(f"SKIP: comment {moment} already exists for issue {issue_id} (memo {memo_id})")
        continue
      if args.verbose:
        print(f"NEW: comment {moment} already exists for issue {issue_id} (memo {memo_id})")
      comment = re.sub(INLINE_LINK, partial(replace_inline_link, issue_id, memo_id), comment, flags=re.MULTILINE)
      enot.execute("INSERT INTO MemoSheets (MemoId, Data, Created, Updated, Station) VALUES (?, ?, ?, ?, ?)",
        (memo_id, comment, moment, moment, STATION_NAME))
      has_comments = True
      COUNT_COMMENST += 1
    if has_comments:
      enot_conn.commit()

  print("===============================================")
  print("Done")
  print(f"  Written memos: {COUNT_MEMOS}")
  print(f"  Written props: {COUNT_PROPS}")
  print(f"  Written options: {COUNT_OPTS}")
  print(f"  Written links: {COUNT_LINKS}")
  print(f"  Written history: {COUNT_HISTORY}")
  print(f"  Written comment: {COUNT_COMMENST}")
  print(f"  Corrected inline links: {COUNT_LINKS_INLINE}")

  adeptus_conn.close()
  enot_conn.close()
