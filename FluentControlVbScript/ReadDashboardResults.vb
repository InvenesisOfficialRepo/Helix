Imports Microsoft.VisualBasic
Imports System
Imports System.IO
Imports Tecan.Core.Scripting

Public Class TestClass
    Implements IScriptObject

    Private Host As IScriptingHost

    Public Property ScriptingHost() As IScriptingHost Implements IScriptObject.ScriptingHost
        Get
            Return Host
        End Get
        Set(ByVal value As IScriptingHost)
            Host = value
        End Set
    End Property

    Private Sub LogMessage(ByVal msg As String)
        Dim logFile As String = "Z:\Evo_pc\Fluent\Experiments\VBScript_Log.txt"
        Try
            File.AppendAllText(logFile, msg & vbCrLf)
        Catch ex As Exception
        End Try
    End Sub

    Public Sub Execute() Implements IScriptObject.Execute
        Dim logFile As String = "Z:\Evo_pc\Fluent\Experiments\VBScript_Log.txt"
        Try
            File.Delete(logFile)
        Catch ex As Exception
        End Try

        LogMessage("--- Starting HelixExperimentConfig.json Parsing ---")

        ' --- 1. Path to HelixExperimentConfig.json ---
        Dim jsonPath As String = "Z:\Evo_pc\Fluent\Experiments\HelixExperimentConfig.json"

        If Not File.Exists(jsonPath) Then
            LogMessage("ERROR: HelixExperimentConfig.json not found: " & jsonPath)
            Throw New Exception("HelixExperimentConfig.json not found: " & jsonPath)
        End If

        Dim jsonText As String = File.ReadAllText(jsonPath)
        If jsonText = "" Then
            LogMessage("ERROR: HelixExperimentConfig.json is empty: " & jsonPath)
            Throw New Exception("HelixExperimentConfig.json is empty: " & jsonPath)
        End If
        
        Try
            File.Delete(jsonPath)
            LogMessage("Successfully read and deleted JSON file.")
        Catch ex As Exception
            LogMessage("Warning: Could not delete JSON file: " & ex.Message)
        End Try

        ' --- 2. Set Strings ---
        SetVarIfFound(jsonText, "Project", False)
        SetVarIfFound(jsonText, "RootPath", False)
        SetVarIfFound(jsonText, "RequestedTest", False)
        SetVarIfFound(jsonText, "Experiment", False)
        SetVarIfFound(jsonText, "QC_Plate_Type", False)
        SetVarIfFound(jsonText, "MatrixLoopCounts", False)

        ' --- 3. Set Integers ---
        SetVarIfFound(jsonText, "NbDaughter", True)
        SetVarIfFound(jsonText, "NbMatrix", True)
        SetVarIfFound(jsonText, "is96", True)
        SetVarIfFound(jsonText, "isMTA", True)

        LogMessage("--- Finished Parsing Successfully ---")
    End Sub

    Private Sub SetVarIfFound(ByVal jsonText As String, ByVal keyName As String, ByVal isInteger As Boolean)
        LogMessage("Attempting to extract: " & keyName)
        Dim val As String = ExtractValue(jsonText, keyName)
        
        If val = "" Then
            LogMessage("  -> Not found or empty in JSON.")
            Return
        End If

        LogMessage("  -> Found value: '" & val & "'")
        
        Try
            If isInteger Then
                Dim intVal As Integer
                If Integer.TryParse(val, intVal) Then
                    Host.SetVariable(keyName, intVal)
                    LogMessage("  -> Successfully set as Integer: " & intVal.ToString())
                Else
                    LogMessage("  -> ERROR: Could not convert '" & val & "' to Integer.")
                End If
            Else
                Host.SetVariable(keyName, val)
                LogMessage("  -> Successfully set as String.")
            End If
        Catch ex As Exception
            LogMessage("  -> EXCEPTION while setting variable: " & ex.Message)
        End Try
    End Sub
    
    Private Function ExtractValue(ByVal jsonText As String, ByVal keyName As String) As String
        Dim keyToken As String = Chr(34) & keyName & Chr(34)
        Dim keyPos As Integer = jsonText.IndexOf(keyToken)

        If keyPos < 0 Then
            Return ""
        End If

        Dim pos As Integer = keyPos + keyToken.Length
        Dim keepGoing As Boolean = True
        While pos < jsonText.Length And keepGoing
            Dim c As Char = jsonText(pos)
            If c = " "c Or c = ":"c Or c = Chr(9) Or c = Chr(13) Or c = Chr(10) Then
                pos = pos + 1
            Else
                keepGoing = False
            End If
        End While

        If pos >= jsonText.Length Or jsonText(pos) <> Chr(34) Then
            Return ""
        End If

        pos = pos + 1
        Dim result As String = ""

        While pos < jsonText.Length
            Dim ch As Char = jsonText(pos)
            If ch = Chr(34) Then
                Exit While
            ElseIf ch = "\"c Then
                pos = pos + 1
                If pos < jsonText.Length Then
                    result = result & jsonText(pos)
                End If
            Else
                result = result & ch
            End If
            pos = pos + 1
        End While

        Return result.Trim()
    End Function

End Class