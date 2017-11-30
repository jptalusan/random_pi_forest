import csv
import pandas as pd
from datetime import datetime

classificationColumn = 1

data = pd.read_csv("df.csv")
headers = data.columns.get_values()
#print(headers.tolist())

strCol = data[headers[classificationColumn]]
keys = strCol.unique()

vals = []
for idx, x in enumerate(keys):
    vals.append(idx)
print(vals)
dictionary = dict(zip(keys, vals))
print(dictionary)

header = headers[classificationColumn]

#Replacing a string activity with int via a dictionary
data[header].replace(dictionary, inplace=True)

#Replacing date time with the format below to %H%M
#2017-08-17 18:45
data["time"] = data["time"].apply(lambda x:datetime.strptime(x, "%Y-%m-%d %H:%M:%S").strftime('%H%M'))

#Saving the newly cleaned csv
data.to_csv("cleaned.csv", header=False, index=False)
