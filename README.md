# GeniusBooks

A Windows C++17 console application for managing a book shop.

## Features

- Add books with automatic IDs, titles, authors, prices, and stock quantities.
- List books and search by title, author (case insensitive), or exact ID.
- Update titles, authors, prices, and stock.
- Record confirmed sales, automatically reducing stock and preventing overselling.
- View dated sales history, quantities sold, and total revenue. Historical titles
  and prices remain unchanged when a book is edited.
- Save every completed change and reload it on the next launch.

## Build and run

Open `ConsoleApplication1.sln` in Visual Studio 2022 with the **Desktop development
with C++** workload and Windows SDK installed. Select Debug or Release and x64,
then press Ctrl+F5. The project enables C++17.

Alternatively, from a Visual Studio developer command prompt:

```bat
msbuild ConsoleApplication1.sln /p:Configuration=Release /p:Platform=x64
x64\Release\ConsoleApplication1.exe
```

Choose a numbered menu option and follow the prompts. The stock update sets the
total quantity currently available. Prices use the shop's chosen currency, with
two decimal places; monetary values are stored as integer cents. Prices and
stock are limited to 1,000,000. Sale confirmation requires `y`; other answers cancel.
Closing input during an operation discards that unfinished operation.

## Data

By default, `geniusbooks.data` is stored beside the executable, independently of
the working directory. To use a different location, pass a file path:

```bat
ConsoleApplication1.exe "C:\BookShop\shop.data"
```

The containing folder must already exist and be writable. Back up the data file
while the app is closed. Stock and sales are saved together through a temporary
file and replacement; failed saves leave the current in-memory state unchanged.
A file lock prevents simultaneous instances using the same data file. Invalid
data stops startup instead of silently discarding or overwriting records.
Data and local build tools are ignored by Git.

## Workflow tests

After building, run this in PowerShell from the repository directory:

```powershell
./tests/workflows.ps1 -Executable ./x64/Release/ConsoleApplication1.exe
```

Tests use an isolated temporary folder and cover book updates, search, exact
decimal sales, stock validation, cancellation, persistence, invalid files,
interrupted input, and failed-save rollback.
