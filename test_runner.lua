#!/usr/bin/env lua

-- Colori ANSI per il terminale
local GREEN = "\27[32m"
local RED   = "\27[31m"
local YELLOW = "\27[33m"
local BOLD  = "\27[1m"
local RESET = "\27[0m"

local COMPILER = "./out/out"
local TEST_DIR = "tests"

-- Funzione per leggere un file di testo per intero
local function read_file(path)
    local f = io.open(path, "r")
    if not f then return nil end
    local content = f:read("*a")
    f:close()
    return content
end

-- Estrae i metadati (EXPECT, INPUT, EXPECT_ERROR) dai commenti del file
local function parse_test_metadata(content)
  local metadata = {
    expected_output = {},
    input = {},
    expect_error = false
  }

  local current_section = nil

  -- (.-)\r?\n legge OGNI singola riga, comprese quelle vuote!
  for line in (content .. "\n"):gmatch("(.-)\r?\n") do
    local comment = line:match("^%s*//%s*(.*)$")
    if comment then
      if comment:match("^EXPECT:") then
        current_section = "EXPECT"
      elseif comment:match("^INPUT:") then
        current_section = "INPUT"
      elseif comment:match("^EXPECT_ERROR") then
        metadata.expect_error = true
        current_section = nil
      elseif current_section == "EXPECT" then
        table.insert(metadata.expected_output, comment)
      elseif current_section == "INPUT" then
        table.insert(metadata.input, comment)
      end
    else
      -- Una riga vuota o di codice chiude la sezione EXPECT!
      current_section = nil
      if not line:match("^%s*$") then
        break
      end
    end
  end

  return metadata
end

-- Rimuove spazi e newline finali.
-- Le parentesi esterne (...) forzano Lua a restituire SOLO 1 valore (evitando il bug del conteggio di gsub!)
local function trim(s)
    return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end

-- Trova tutti i file .txt nella cartella tests
local function get_test_files()
    local files = {}
    local p = io.popen("find " .. TEST_DIR .. " -maxdepth 1 -name '*.txt' | sort")
    if p then
        for file in p:lines() do
            table.insert(files, file)
        end
        p:close()
    end
    return files
end

-- Esecuzione principale
local function run_tests()
    print(BOLD .. "================ TEST RUNNER (compilerc) ================" .. RESET)

    local test_files = get_test_files()
    if #test_files == 0 then
        print(YELLOW .. "Nessun file trovato in " .. TEST_DIR .. "/" .. RESET)
        return
    end

    local passed = 0
    local failed = 0

    for _, test_path in ipairs(test_files) do
        local test_name = test_path:match("([^/]+)%.txt$")
        local content = read_file(test_path)
        local meta = parse_test_metadata(content)

        -- 1. Compilazione
        local compile_cmd = string.format("%s %s 2>&1", COMPILER, test_path)
        local pipe = io.popen(compile_cmd)
        local compile_output = pipe:read("*a")
        local ok, exit_type, code = pipe:close()
        local compile_success = (exit_type == "exit" and code == 0) or (ok == true)

        -- Caso A: Test che DOVEVA fallire la compilazione (es. variabile non definita)
        if meta.expect_error then
            if not compile_success then
                print(string.format("%s[PASS]%s %-30s (Errore di compilazione catturato correttamente)", GREEN, RESET, test_name))
                passed = passed + 1
            else
                print(string.format("%s[FAIL]%s %-30s (Doveva fallire la compilazione, ma ha avuto successo!)", RED, RESET, test_name))
                failed = failed + 1
            end
        -- Caso B: Test che deve compilare ed eseguire con successo
        else
            if not compile_success then
                print(string.format("%s[FAIL]%s %-30s (Compilazione fallita)", RED, RESET, test_name))
                print(YELLOW .. "Output del compilatore:\n" .. compile_output .. RESET)
                failed = failed + 1
            else
                -- 2. Esecuzione del binario compilato
                local bin_path = test_path:gsub("%.txt$", "")
                local input_str = table.concat(meta.input, "\n")
                if #meta.input > 0 then input_str = input_str .. "\n" end

                local exec_cmd = string.format("echo '%s' | ./%s 2>&1", input_str, bin_path)
                local exec_pipe = io.popen(exec_cmd)
                local actual_output = exec_pipe:read("*a")
                exec_pipe:close()

                -- Pulizia file binario e file .s generati
                os.remove(bin_path)
                os.remove(bin_path .. ".s")

                -- Pulizia dell'output: togliamo il prompt di scan (es. "scan a = ") se presente
                local actual_lines = {}
                for line in actual_output:gmatch("[^\r\n]+") do
                    local cleaned = trim(line)
                    -- Rimuove il prefisso "scan nome = " se c'è un input
                    cleaned = cleaned:gsub("^scan%s+.-=%s*", "")
                    if cleaned ~= "" then
                        table.insert(actual_lines, cleaned)
                    end
                end

                local expected_str = trim(table.concat(meta.expected_output, "\n"))
                local actual_str   = trim(table.concat(actual_lines, "\n"))

                if expected_str == actual_str then
                    print(string.format("%s[PASS]%s %-30s", GREEN, RESET, test_name))
                    passed = passed + 1
                else
                    print(string.format("%s[FAIL]%s %-30s", RED, RESET, test_name))
                    print(YELLOW .. "  Atteso:\n" .. expected_str .. RESET)
                    print(RED .. "  Ottenuto:\n" .. actual_str .. RESET)
                    failed = failed + 1
                end
            end
        end
    end

    print(BOLD .. "==========================================================" .. RESET)
    local summary_color = (failed == 0) and GREEN or RED
    print(string.format("%sRisultato finale: %d Passati, %d Falliti su %d test totali%s\n",
        summary_color, passed, failed, #test_files, RESET))
    if failed > 0 then
        os.exit(1)
    end
end

run_tests()
