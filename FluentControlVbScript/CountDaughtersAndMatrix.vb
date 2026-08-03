Imports Microsoft.VisualBasic
Imports System
Imports System.IO
Imports System.Collections.Generic
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

    Public Sub Execute() Implements IScriptObject.Execute
        ' ==== ADJUST THIS IF YOU WANT MORE/Fewer SLOTS ====
        Const MAX_SLOTS As Integer = 32

        Dim experiment As String = Convert.ToString(Host.GetVariable("Experiment"))
        If String.IsNullOrEmpty(experiment) Then
            Throw New ArgumentException("FluentControl variable 'Experiment' is empty or not set.")
        End If

        Dim basePath As String = "Z:\Fluent\Experiments"
        Dim expPath As String = Path.Combine(basePath, experiment)

        Dim nbDaughterActual As Integer = 0
        Dim nbMatrixTotal As Integer = 0

        ' Collect GLOBAL barcodes to write later (case-insensitive de-dup)
        Dim barcodeList As New List(Of String)()

        Try
            If Not Directory.Exists(expPath) Then
                Throw New DirectoryNotFoundException("Experiment folder not found: " & expPath)
            End If

            ' Collect daughter dirs (support both dght_* and dgth_ just in case)
            Dim candidates() As String = Directory.GetDirectories(expPath, "*", SearchOption.TopDirectoryOnly)
            Dim daughterDirs As New List(Of String)()
            Dim seen As New List(Of String)()

            Dim i As Integer
            For i = 0 To candidates.Length - 1
                Dim d As String = candidates(i)
                Dim name As String = Path.GetFileName(d)
                Dim lname As String = name.ToLowerInvariant()
                If lname.StartsWith("dght_") OrElse lname.StartsWith("dgth_") Then
                    ' de-dup case-insensitively
                    Dim dupe As Boolean = False
                    Dim s As String
                    For Each s In seen
                        If s = d.ToLowerInvariant() Then
                            dupe = True : Exit For
                        End If
                    Next
                    If Not dupe Then
                        daughterDirs.Add(d)
                        seen.Add(d.ToLowerInvariant())
                    End If
                End If
            Next

            ' Sort by numeric suffix if present (dght_0, dght_1...), else alphabetical
            daughterDirs.Sort(New DghtComparer())

            nbDaughterActual = daughterDirs.Count
            TrySetVar("NbDaughter", nbDaughterActual)

            ' Clear all slots first to avoid stale data
            For i = 0 To MAX_SLOTS - 1
                TrySetVar("NbMatrix_" & i.ToString(), 0)
                TrySetVar("DghtName_" & i.ToString(), "")
                ' optional helpers (only if predeclared)
                TrySetVar("NbMatrixUnique_" & i.ToString(), 0)
                TrySetVar("ExpectedMatrixListPath_" & i.ToString(), "")
                TrySetVar("ExpectedBarcodeCSV_" & i.ToString(), "")
            Next

            ' Fill up to MAX_SLOTS and collect PHC barcodes while we loop
            Dim slotsUsed As Integer = Math.Min(nbDaughterActual, MAX_SLOTS)
            TrySetVar("NbDaughterUsed", slotsUsed) ' optional helper if you predeclared it

            For i = 0 To slotsUsed - 1
                Dim d As String = daughterDirs(i)
                Dim folderName As String = Path.GetFileName(d)

                Dim phcFiles As String() = Directory.GetFiles(d, "PHC*.gwl", SearchOption.TopDirectoryOnly)
                Dim count As Integer = phcFiles.Length

                ' Per-daughter barcode list (case-insensitive de-dup)
                Dim folderBarcodeList As New List(Of String)()

                ' collect stems for expected_matrix_barcodes.txt (GLOBAL + PER-FOLDER)
                Dim f As String
                For Each f In phcFiles
                    Dim stem As String = Path.GetFileNameWithoutExtension(f) ' e.g., "PHC0006645"

                    ' global de-dup
                    If Not ContainsCIList(barcodeList, stem) Then
                        barcodeList.Add(stem)
                    End If

                    ' per-folder de-dup
                    If Not ContainsCIList(folderBarcodeList, stem) Then
                        folderBarcodeList.Add(stem)
                    End If
                Next

                nbMatrixTotal += count

                TrySetVar("NbMatrix_" & i.ToString(), count)
                TrySetVar("DghtName_" & i.ToString(), folderName)

                ' optional: unique count per folder
                TrySetVar("NbMatrixUnique_" & i.ToString(), folderBarcodeList.Count)

                ' ===== Build expected_matrix_barcodes.txt INSIDE THIS dght_* folder =====
                folderBarcodeList.Sort(StringComparer.OrdinalIgnoreCase)
                Dim perDghtOutPath As String = WriteExpectedListFile(d, folderBarcodeList, "expected_matrix_barcodes.txt")

                ' optional helpers (only if predeclared)
                TrySetVar("ExpectedMatrixListPath_" & i.ToString(), perDghtOutPath)
                TrySetVar("ExpectedBarcodeCSV_" & i.ToString(), String.Join(";", folderBarcodeList.ToArray()))
            Next

            ' Global counts
            TrySetVar("NbMatrix", barcodeList.Count)          ' unique PHC stems across ALL dght folders
            TrySetVar("NbMatrixFiles", nbMatrixTotal)         ' total PHC*.gwl files across ALL dght folders

            ' ===== Build expected_matrix_barcodes.txt at experiment root (GLOBAL) =====
            barcodeList.Sort(StringComparer.OrdinalIgnoreCase)
            Dim outPath As String = WriteExpectedListFile(expPath, barcodeList, "expected_matrix_barcodes.txt")

            ' Optional helper variables (only if predeclared)
            TrySetVar("ExpectedMatrixListPath", outPath)
            TrySetVar("ExpectedBarcodeCSV", String.Join(";", barcodeList.ToArray()))
            TrySetVar("CreatedExpectedList", True)

        Catch ex As Exception
            TrySetVar("CreatedExpectedList", False)
            TrySetVar("ScriptError", ex.Message)
            Throw
        End Try
    End Sub

    ' Writes a simple newline-separated barcode file in the given folder and returns the full path.
    Private Function WriteExpectedListFile(ByVal folderPath As String, ByVal barcodes As List(Of String), ByVal filename As String) As String
        Dim outPath As String = Path.Combine(folderPath, filename)

        Dim sw As StreamWriter = Nothing
        Try
            sw = New StreamWriter(outPath, False)
            Dim bc As String
            For Each bc In barcodes
                sw.WriteLine(bc)
            Next
        Finally
            If Not sw Is Nothing Then
                Try : sw.Flush() : Catch : End Try
                Try : sw.Close() : Catch : End Try
            End If
        End Try

        Return outPath
    End Function

    ' Safe setter: writes only if the variable exists (predeclared in FluentControl)
    Private Sub TrySetVar(ByVal name As String, ByVal value As Object)
        Try
            Host.SetVariable(name, value)
        Catch
            ' Ignore if the variable wasn't predeclared
        End Try
    End Sub

    ' Case-insensitive list membership (avoids HashSet dependency)
    Private Function ContainsCIList(ByVal items As List(Of String), ByVal value As String) As Boolean
        Dim v As String = value.ToLowerInvariant()
        Dim s As String
        For Each s In items
            If s.ToLowerInvariant() = v Then Return True
        Next
        Return False
    End Function

    ' Comparer that sorts dght_XX by numeric suffix if present
    Private Class DghtComparer
        Implements IComparer(Of String)

        Public Function Compare(ByVal x As String, ByVal y As String) As Integer Implements IComparer(Of String).Compare
            Dim nx As Integer = GetIndex(Path.GetFileName(x))
            Dim ny As Integer = GetIndex(Path.GetFileName(y))

            If nx <> Integer.MinValue AndAlso ny <> Integer.MinValue Then
                Dim c As Integer = nx.CompareTo(ny)
                If c <> 0 Then Return c
            End If

            Return String.Compare(x, y, StringComparison.OrdinalIgnoreCase)
        End Function

        Private Function GetIndex(ByVal name As String) As Integer
            Dim us As Integer = name.LastIndexOf("_"c)
            If us >= 0 AndAlso us < name.Length - 1 Then
                Dim suf As String = name.Substring(us + 1)
                Dim n As Integer
                If Integer.TryParse(suf, n) Then Return n
            End If
            Return Integer.MinValue
        End Function
    End Class
End Class
