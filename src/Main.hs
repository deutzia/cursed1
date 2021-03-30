{-# Options -Wall -Wname-shadowing #-}

module Main where

import qualified Data.Map as M
import System.Environment
import System.IO
import Control.Monad.Except
import System.Exit
import Data.Bits
import Data.Maybe as Maybe
import System.Process

data Direction = In | Out deriving Show
data Type = Void | Int_ | Uint | Long | Ulong | Longlong | Ulonglong | Ptr deriving (Show, Eq)
-- FunType RetType [ArgTypes]
data FunType = FunType Type [Type] deriving Show
type FunFullType = (String, FunType)

type ParseM = Except String

trampolinePrefix :: String
trampolinePrefix = "_ZSO_trampoline_"

saveRegisters :: String
saveRegisters =
    "\tpush rbx\n" ++
    "\tpush rbp\n" ++
    "\tpush r12\n" ++
    "\tpush r13\n" ++
    "\tpush r14\n" ++
    "\tpush r15\n"

restoreRegisters :: String
restoreRegisters =
    "\tpop r15\n" ++
    "\tpop r14\n" ++
    "\tpop r13\n" ++
    "\tpop r12\n" ++
    "\tpop rbp\n" ++
    "\tpop rbx\n"

parseType :: String -> ParseM Type
parseType "void" = return Void
parseType "int" = return Int_
parseType "uint" = return Uint
parseType "long" = return Long
parseType "ulong" = return Ulong
parseType "longlong" = return Longlong
parseType "ulonglong" = return Ulonglong
parseType "ptr" = return Ptr
parseType t = throwError $ "Unknown type: " ++ t

parseArgType :: String -> ParseM Type
parseArgType t =
    parseType t >>=
        (\t' -> if t' == Void
            then throwError "Cannot have argumentb of type void"
            else return t')

parseLine :: String -> ParseM FunFullType
parseLine line =
    let w = words line in
    case w of
        fName : (retType : argTypes) -> (\ft -> (fName, ft)) <$> ((FunType <$> parseType retType) <*> mapM parseArgType argTypes)
        _ -> throwError "Line should have at least two words: function name and returned type"

parseFlist :: String -> ParseM [FunFullType]
parseFlist contents =
    let linesToParse = filter (not . null) (lines contents) in
    mapM parseLine linesToParse

parseDirection :: String -> ParseM Direction
parseDirection "in" = return In
parseDirection "out" = return Out
parseDirection t = throwError $ "Unknown direction: " ++ t

parseConverterOutLine :: String -> ParseM (String, Direction)
parseConverterOutLine line =
    let w = words line in
    case w of
        [fName, dir] -> (\d -> (fName, d)) <$> parseDirection dir
        _ -> throwError "Every line should have exactly two words"

parseConverterOut :: String -> ParseM [(String, Direction)]
parseConverterOut contents =
    let linesToParse = filter (not . null) (lines contents) in
    mapM parseConverterOutLine linesToParse

getEnv :: String -> String -> ParseM ((M.Map String FunType), [(String, Direction)])
getEnv flist converter_out = do
    flist' <- parseFlist flist
    dirs <- parseConverterOut converter_out
    let m = M.fromList flist'
    mapM_
        (\(s, _) -> case M.lookup s m of
            Just _ -> return ()
            Nothing -> throwError $ "Trampoline should be generated for " ++ s ++ " but there is no type signature for it"
        )
        dirs
    return (m, dirs)

trampolinePrelude :: [(String, Direction)] -> String
trampolinePrelude =
    let
        helper (name, In) =
            "global " ++ name ++ "\n" ++
            "extern " ++ trampolinePrefix ++ name ++ "\n"
        helper (_, Out) = ""
    in concatMap helper

trampolineEntryPoint32Name :: String -> String
trampolineEntryPoint32Name name = trampolinePrefix ++ "32_" ++ name

trampolineEntryPoint64Name :: String -> String
trampolineEntryPoint64Name name = trampolinePrefix ++ "64_" ++ name

argRegisters :: [(String, String)]
argRegisters = [("rdi", "edi"), ("rsi", "esi"), ("rdx", "edx"), ("rcx", "ecx"), ("r8", "r8d"), ("r9", "r9d")]

saveArgs :: [Type] -> String
saveArgs args =
    "\tsub rsp, " ++ show (sizeOnStackAligned args) ++ "\n" ++
    let
        (_, movs) = foldr
            (\(t, (wide, slim)) (off, acc) ->
                let s = sizeOf32 t in
                (off + s,
                acc ++ "\tmov [rsp + " ++ show off ++ "], " ++
                (if s == 8 then wide else slim) ++ "\n"))
            (0, "")
            (zip args argRegisters)
    in movs

jumpTo32 :: String -> String
jumpTo32 name =
    "\tsub rsp, 4\n" ++
    "\tmov dword [rsp], 0x23\n" ++
    "\tsub rsp, 4\n" ++
    "\tmov dword [rsp], " ++ trampolineEntryPoint32Name name ++ "\n" ++
    "\tretf\n"

jumpTo64 :: String -> String
jumpTo64 name =
    "\tsub esp, 4\n" ++
    "\tmov dword [esp], 0x33\n" ++
    "\tsub esp, 4\n" ++
    "\tmov dword [esp], " ++ trampolineEntryPoint64Name name ++ "\n" ++
    "\tretf\n"

sizeOf32 :: Type -> Int
sizeOf32 Void = undefined
sizeOf32 Int_ = 4
sizeOf32 Uint = 4
sizeOf32 Long = 4
sizeOf32 Ulong = 4
sizeOf32 Longlong = 8
sizeOf32 Ulonglong = 8
sizeOf32 Ptr = 4

sizeOnStack :: [Type] -> Int
sizeOnStack = foldl (\acc t -> acc + sizeOf32 t) 0

sizeOnStackAligned :: [Type] -> Int
sizeOnStackAligned l = ((sizeOnStack l -1) .|. 0xf) + 1

trampolineInEntryPoint :: String -> FunType -> String
trampolineInEntryPoint name (FunType _retType argTypes) =
    name ++ ":\n" ++
    "\t[bits 64]\n" ++
    saveRegisters ++
    saveArgs argTypes ++
    jumpTo32 name ++
    "\n"

trampolineInEntryPoint32 :: String -> String
trampolineInEntryPoint32 name =
    trampolineEntryPoint32Name name ++ ":\n" ++
    "\t[bits 32]\n" ++
    "\tpush dword 0x2b\n" ++
    "\tpop ds\n" ++
    "\tpush dword 0x2b\n" ++
    "\tpop es\n" ++
    "\tcall " ++ trampolinePrefix ++ name ++ "\n" ++
    jumpTo64 name ++
    "\n"

convertReturned32To64 :: Type -> String
convertReturned32To64 Void = ""
convertReturned32To64 Int_ =
    "\tmovsxd rdx, eax\n" ++
    "\tmov rax, rdx\n"
convertReturned32To64 Uint =
    "\tmov eax, eax\n"
convertReturned32To64 Long = convertReturned32To64 Int_
convertReturned32To64 Ulong = convertReturned32To64 Uint
convertReturned32To64 Longlong =
    "\tmov eax, eax\n" ++
    "\tshl rdx, 32\n" ++
    "\t or rax, rdx\n"
convertReturned32To64 Ulonglong = convertReturned32To64 Longlong
convertReturned32To64 Ptr = convertReturned32To64 Uint

trampolineInEntryPoint64 :: String -> FunType -> String
trampolineInEntryPoint64 name (FunType retType argTypes) =
    trampolineEntryPoint64Name name ++ ":\n" ++
    "\t[bits 64]\n" ++
    convertReturned32To64 retType ++
    "\tadd rsp, " ++ show (sizeOnStackAligned argTypes) ++ "\n" ++
    restoreRegisters ++
    "\n"

generateTrampolines :: (M.Map String FunType, [(String, Direction)]) -> String
generateTrampolines (types, converterOut) =
    let
        helper (name, In) =
            let t = Maybe.fromJust (M.lookup name types) in
            trampolineInEntryPoint name t ++
            trampolineInEntryPoint32 name ++
            trampolineInEntryPoint64 name t ++
            "\n"
        helper (_, Out) = ""
    in
    "default rel\n" ++
    "bits 64\n\n" ++
    trampolinePrelude converterOut ++
    "\n" ++
    concatMap helper converterOut

main :: IO ()
main = do
    args <- getArgs
    case args of
        [origElf, flist, converterOut, asmFname, tempElf, tempElf2, resultElf] -> do
--            callCommand $ "./main " ++ origElf ++ " " ++ flist ++ " " ++ tempElf ++ " > " ++ converterOut
            flist' <- readFile flist
            converterOut' <- readFile converterOut
            case runExcept (Main.getEnv flist' converterOut') of
                Left err -> hPutStrLn stderr err >> exitFailure
                Right parseResult -> do
                    writeFile asmFname $ generateTrampolines parseResult
--                    callCommand $ "nasm -g -f elf64 " ++ asmFname ++ " -o " ++ tempElf2
--                    callCommand $ "ld -r " ++ tempElf2 ++ " " ++ tempElf ++ " -o " ++ resultElf
                    exitSuccess
        _ -> hPutStrLn stderr "Incorrect usage" >> exitFailure
