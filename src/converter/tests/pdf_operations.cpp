/* Direct integration tests using the installed, hash-pinned qpdf distribution. */
#include "pdfprocessor.h"
#include "qpdfbundle.h"
#include "nativefiletransaction.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <QCryptographicHash>
#include <functional>
#include <cstdio>
#include <stdexcept>
#include <vector>
using namespace au::converter;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
QByteArray read(const QString& path) { QFile file(path); require(file.open(QIODevice::ReadOnly), "read fixture"); return file.readAll(); }
void put(const QString& path, const QByteArray& value) {
    QFile file(path); require(file.open(QIODevice::WriteOnly), "create fixture"); require(file.write(value) == value.size(), "write fixture");
}
QByteArray qpdf(const QStringList& args) {
    QProcess p; p.setProgram(PdfProcessor::bundledToolPath()); p.setArguments(args); p.start();
    require(p.waitForStarted(5000) && p.waitForFinished(10000), "independent qpdf oracle completed");
    if (p.exitCode() != 0) throw std::runtime_error(p.readAllStandardError().toStdString());
    return p.readAllStandardOutput();
}
void pdf(const QString& path, const QVector<int>& ids, bool info = false) {
    QVector<QByteArray> objects {"<< /Type /Catalog /Pages 2 0 R >>", {}};
    QByteArray kids;
    for (int id : ids) {
        const int object = objects.size() + 1;
        kids += QByteArray::number(object) + " 0 R ";
        const QByteArray text = "q 0 0 1 rg 10 10 " + QByteArray::number(id) + " 20 re f Q\n";
        objects << ("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " + QByteArray::number(200 + id)
                    + " " + QByteArray::number(300 + id) + "] /Resources << >> /TestIdentity (page-" + QByteArray::number(id)
                    + ") /Contents " + QByteArray::number(object + 1) + " 0 R >>");
        objects << ("<< /Length " + QByteArray::number(text.size()) + " >>\nstream\n" + text + "endstream");
    }
    objects[1] = "<< /Type /Pages /Count " + QByteArray::number(ids.size()) + " /Kids [" + kids + "] >>";
    if (info) objects << "<< /Title (old) /Producer (retained producer) >>";
    QByteArray output = "%PDF-1.7\n"; QVector<int> offsets {0};
    for (int i = 0; i < objects.size(); ++i) { offsets << output.size(); output += QByteArray::number(i + 1) + " 0 obj\n" + objects[i] + "\nendobj\n"; }
    const int xref = output.size(); output += "xref\n0 " + QByteArray::number(objects.size() + 1) + "\n0000000000 65535 f \n";
    for (int i = 1; i < offsets.size(); ++i) output += QByteArray::number(offsets[i]).rightJustified(10, '0') + " 00000 n \n";
    output += "trailer\n<< /Size " + QByteArray::number(objects.size() + 1) + " /Root 1 0 R";
    if (info) output += " /Info " + QByteArray::number(objects.size()) + " 0 R";
    output += " >>\nstartxref\n" + QByteArray::number(xref) + "\n%%EOF\n"; put(path, output);
}
QJsonObject json(const QString& path) { return QJsonDocument::fromJson(qpdf({QStringLiteral("--json"), path})).object(); }
void identities(const QString& path, const QVector<int>& ids, int rotation = 0) {
    const auto document = json(path); const auto pages = document.value("pages").toArray();
    const auto table = document.value("qpdf").toArray()[1].toObject();
    require(pages.size() == ids.size(), "oracle page count");
    for (int i = 0; i < pages.size(); ++i) {
        const auto object = table.value("obj:" + pages[i].toObject().value("object").toString()).toObject().value("value").toObject();
        require(object.value("/TestIdentity").toString() == QStringLiteral("u:page-%1").arg(ids[i]), "page identity and order");
        const auto box = object.value("/MediaBox").toArray();
        require(box.size() == 4 && box[2].toInt() == 200 + ids[i] && box[3].toInt() == 300 + ids[i], "exact page dimensions");
        require((object.value("/Rotate").toInt() % 360) == rotation, "exact page rotation");
        const auto streams = pages[i].toObject().value("contents").toArray(); require(streams.size() == 1, "page retains content stream");
        const auto number = streams[0].toString().section(' ', 0, 0);
        const auto content = qpdf({"--show-object=" + number, "--filtered-stream-data", path});
        require(content.contains("10 10 " + QByteArray::number(ids[i]) + " 20 re f"), "page content identity");
    }
}
QJsonObject info(const QString& path) {
    const auto table = json(path).value("qpdf").toArray()[1].toObject();
    const auto reference = table.value("trailer").toObject().value("value").toObject().value("/Info").toString();
    return table.value("obj:" + reference).toObject().value("value").toObject();
}
struct Fixture {
    QTemporaryDir folder;
    QString source = folder.filePath("source.pdf");
    QString output = folder.filePath("result.pdf");
    QByteArray original;
    Fixture() { require(folder.isValid(), "fixture directory"); pdf(source, {1,2,3,4,5}); original = read(source); }
    PdfRequest request(PdfOperation operation) { PdfRequest r; r.operation = operation; r.sourcePaths = {source}; r.outputPath = output; return r; }
    PdfResult run(PdfRequest r, const std::atomic_bool* cancel = nullptr) { return PdfProcessor().process(r, cancel); }
    void preserved() {
        require(read(source) == original, "original bytes unchanged");
        require(QDir(folder.path()).entryList({".audacity-convert-*"}, QDir::Files | QDir::Hidden).isEmpty(), "all owned unpublished temporaries removed");
    }
    void success(const PdfResult& r, int count, int outputs = 1) {
        if (!r.ok) throw std::runtime_error(r.message.toStdString());
        require(!r.cancelled && r.pageCount == count && r.outputs.size() == outputs, "complete success output ledger");
        for (const auto& output : r.outputs) require(output.committed && QFileInfo::exists(output.path), "committed output exists");
        preserved();
    }
};
struct Reset {
    ~Reset() { PdfProcessor::testHook = {}; PdfProcessor::testTimeoutMilliseconds = PdfProcessor::TimeoutMilliseconds;
        PdfProcessor::testProcessOutputBytes = 1024 * 1024; PdfProcessor::testOutputBytes = PdfProcessor::MaxOutputBytes;
        PdfProcessor::testProcessMemoryBytes = PdfProcessor::MaxProcessMemoryBytes; }
};
}
int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    const std::vector<std::pair<const char*, std::function<void()>>> tests {
        {"complete official bundle and independent lock agree", [] {
            const auto lock = QJsonDocument::fromJson(read(QString::fromUtf8(AU_QPDF_LOCK_FILE))).object().value("files").toObject();
            require(lock.size() == detail::QpdfFiles.size() && lock.size() == 10, "complete executable and DLL inventory");
            for (auto it = detail::QpdfFiles.cbegin(); it != detail::QpdfFiles.cend(); ++it) require(lock.value(it.key()).toString() == it.value(), "runtime pins match independent lock");
            if (!PdfProcessor::available()) throw std::runtime_error(PdfProcessor::availabilityReason().toStdString());
            require(qpdf({"--version"}).startsWith("qpdf version 12.3.2"), "actual pinned version");
        }},
        {"inspect reopens unique pages and protects original", [] { Fixture f; auto r=f.run(f.request(PdfOperation::Inspect)); f.success(r,5,0); identities(f.source,{1,2,3,4,5}); }},
        {"split preserves chunk identities counts and order", [] { Fixture f; auto r=f.request(PdfOperation::Split); r.pageSpec="2"; auto result=f.run(r); f.success(result,5,3); identities(result.outputs[0].path,{1,2}); identities(result.outputs[1].path,{3,4}); identities(result.outputs[2].path,{5}); require(result.outputs[0].pageCount==2 && result.outputs[2].pageCount==1,"per-file split counts"); }},
        {"split twelve pages is numerically ordered", [] { Fixture f; QVector<int> ids; for(int i=1;i<=12;++i) ids<<i; pdf(f.source,ids); f.original=read(f.source); auto r=f.request(PdfOperation::Split); r.pageSpec="1"; auto result=f.run(r); f.success(result,12,12); for(int i=0;i<12;++i) identities(result.outputs[i].path,{i+1}); }},
        {"extract selects exact pages", [] { Fixture f; auto r=f.request(PdfOperation::Extract); r.pageSpec="2,4-5"; f.success(f.run(r),3); identities(f.output,{2,4,5}); }},
        {"reorder keeps reverse ranges and duplicates", [] { Fixture f; auto r=f.request(PdfOperation::Reorder); r.pageSpec="5-3,1,1"; f.success(f.run(r),5); identities(f.output,{5,4,3,1,1}); }},
        {"merge respects source and page order", [] { Fixture f; auto second=f.folder.filePath("second.pdf"); pdf(second,{21,22}); auto secondOriginal=read(second); auto r=f.request(PdfOperation::Merge); r.sourcePaths={second,f.source}; f.success(f.run(r),7); identities(f.output,{21,22,1,2,3,4,5}); require(read(second)==secondOriginal,"second original preserved"); }},
        {"rotate exact ninety degrees", [] { Fixture f; auto r=f.request(PdfOperation::Rotate); r.rotation=90; f.success(f.run(r),5); identities(f.output,{1,2,3,4,5},90); }},
        {"rotate exact one hundred eighty degrees", [] { Fixture f; auto r=f.request(PdfOperation::Rotate); r.rotation=180; f.success(f.run(r),5); identities(f.output,{1,2,3,4,5},180); }},
        {"rotate exact two hundred seventy degrees", [] { Fixture f; auto r=f.request(PdfOperation::Rotate); r.rotation=270; f.success(f.run(r),5); identities(f.output,{1,2,3,4,5},270); }},
        {"metadata creates missing Info with exact Unicode fields", [] { Fixture f; auto r=f.request(PdfOperation::SetMetadata); r.metadata={{"Title",QString::fromUtf8("音樂 🎵 café")},{"Author",QString::fromUtf8("作者 Ångström")},{"Subject",QString::fromUtf8("主題 日本語")},{"Keywords",QString::fromUtf8("聲音, résumé, 🔊")}}; f.success(f.run(r),5); auto values=info(f.output); for(auto it=r.metadata.cbegin();it!=r.metadata.cend();++it) require(values.value("/"+it.key()).toString()=="u:"+it.value(),"exact Unicode metadata round trip"); identities(f.output,{1,2,3,4,5}); }},
        {"metadata updates existing Info and retains other fields", [] { Fixture f; pdf(f.source,{1,2},true); f.original=read(f.source); auto r=f.request(PdfOperation::SetMetadata); r.metadata={{"Title",QString::fromUtf8("新標題 😀")},{"Author",""}}; f.success(f.run(r),2); auto values=info(f.output); require(values.value("/Title")=="u:"+r.metadata["Title"] && values.value("/Author")=="u:" && values.value("/Producer")=="u:retained producer","metadata exact empty and preserved fields"); }},
        {"corrupt PDF is rejected without output", [] { Fixture f; put(f.source,"%PDF-1.7\nnot an object table\n"); f.original=read(f.source); auto r=f.run(f.request(PdfOperation::Inspect)); require(!r.ok && !r.cancelled && !r.message.isEmpty(),"corrupt PDF rejected"); f.preserved(); }},
        {"encrypted PDF rejected without credential argv", [] { Fixture f; const auto encrypted=f.folder.filePath("encrypted.pdf"); const auto job=f.folder.filePath("fixture-job.json");
            QJsonObject encryption{{"userPassword",""},{"ownerPassword","synthetic fixture only"},{"256bit",QJsonObject{}}};
            put(job,QJsonDocument(QJsonObject{{"inputFile",f.source},{"outputFile",encrypted},{"encrypt",encryption}}).toJson());
            qpdf({"--job-json-file="+job}); require(QFile::remove(job),"discard synthetic fixture job");
            auto r=f.request(PdfOperation::Inspect); r.sourcePaths={encrypted}; auto result=f.run(r); require(!result.ok && result.message.contains("Encrypted"),"encrypted fixture refused"); f.preserved(); }},
        {"pre-cancelled operation reads no source", [] { Fixture f; std::atomic_bool c=true; auto result=f.run(f.request(PdfOperation::Inspect),&c); require(!result.ok && result.cancelled,"pre-cancelled result"); f.preserved(); }},
        {"cancellation reaches real inspection subprocess", [] { Fixture f; std::atomic_bool c=false; int started=0; PdfProcessor::testHook=[&](auto phase,const QString&){if(phase==PdfProcessor::TestPhase::ProcessStarted && ++started==2)c=true;}; auto result=f.run(f.request(PdfOperation::Inspect),&c); require(started==2 && !result.ok && result.cancelled,"check process cancellation"); f.preserved(); }},
        {"cancellation during output validation cleans owned temporary", [] { Fixture f; std::atomic_bool c=false; PdfProcessor::testHook=[&](auto phase,const QString& args){if(phase==PdfProcessor::TestPhase::ProcessStarted && args.contains("--check") && args.contains(".audacity-convert-"))c=true;}; auto r=f.request(PdfOperation::Rotate);r.rotation=90;auto result=f.run(r,&c);require(!result.ok&&result.cancelled&&!QFileInfo::exists(f.output),"output check cancellation");f.preserved(); }},
        {"cancellation immediately before publish preserves original", [] {Fixture f;std::atomic_bool c=false;PdfProcessor::testHook=[&](auto phase,const QString&){if(phase==PdfProcessor::TestPhase::BeforePublish)c=true;};auto r=f.request(PdfOperation::Rotate);r.rotation=90;auto result=f.run(r,&c);require(!result.ok&&result.cancelled&&result.outputs.isEmpty()&&!QFileInfo::exists(f.output),"cancel before atomic publish");f.preserved();}},
        {"PDF validation temporary cannot be replaced or overwritten", [] {Fixture f;bool checked=false;PdfProcessor::testHook=[&](auto phase,const QString& args){if(phase!=PdfProcessor::TestPhase::ProcessStarted||!args.startsWith("--check ")||!args.contains(".audacity-convert-"))return;checked=true;const auto path=args.mid(8);require(!DeleteFileW(reinterpret_cast<LPCWSTR>(path.utf16())),"validation file deletion blocked");detail::Handle writer(CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));require(!writer.valid(),"validation file overwrite blocked");};auto r=f.request(PdfOperation::Rotate);r.rotation=90;f.success(f.run(r),5);require(checked,"real output validation barrier");identities(f.output,{1,2,3,4,5},90);}},
        {"existing output collision preserves user bytes", [] { Fixture f; put(f.output,"user-owned contents");auto r=f.request(PdfOperation::Rotate);r.rotation=90;auto result=f.run(r);require(!result.ok&&read(f.output)=="user-owned contents","collision preservation");f.preserved(); }},
        {"late collision preserves destination and deletes owned temporary", [] { Fixture f; PdfProcessor::testHook=[&](auto phase,const QString& path){if(phase==PdfProcessor::TestPhase::BeforePublish)put(path,"late user data");};auto r=f.request(PdfOperation::Rotate);r.rotation=90;auto result=f.run(r);require(!result.ok&&result.outputs.isEmpty()&&read(f.output)=="late user data","late collision");f.preserved(); }},
        {"partial split retains committed first output and user replacement", [] { Fixture f; int publishing=0;QString first;PdfProcessor::testHook=[&](auto phase,const QString& path){if(phase==PdfProcessor::TestPhase::BeforePublish && ++publishing==2){require(QFile::remove(first),"replace previously released output");put(first,"replacement user data");put(path,"collision user data");}if(phase==PdfProcessor::TestPhase::OutputPublished&&first.isEmpty())first=path;};auto r=f.request(PdfOperation::Split);r.pageSpec="2";auto result=f.run(r);require(!result.ok&&result.outputs.size()==1&&result.outputs[0].committed&&result.pageCount==2,"honest committed partial ledger");require(read(first)=="replacement user data"&&read(f.folder.filePath("result-3-4.pdf"))=="collision user data","no path rollback deletion");f.preserved(); }},
        {"partial split cancellation retains first committed output", [] { Fixture f;std::atomic_bool c=false;PdfProcessor::testHook=[&](auto phase,const QString&){if(phase==PdfProcessor::TestPhase::OutputPublished)c=true;};auto r=f.request(PdfOperation::Split);r.pageSpec="2";auto result=f.run(r,&c);require(!result.ok&&result.cancelled&&result.outputs.size()==1&&result.pageCount==2,"partial cancellation ledger");identities(result.outputs[0].path,{1,2});f.preserved(); }},
        {"source and bundle handles prevent mutation during process", [] {Fixture f;bool visited=false;PdfProcessor::testHook=[&](auto phase,const QString&){if(phase!=PdfProcessor::TestPhase::SourcesPinned)return;visited=true;QFile source(f.source);require(!source.open(QIODevice::WriteOnly),"source cannot be overwritten");require(!QFile::rename(f.source,f.source+".moved"),"source cannot be renamed");QFile dll(QFileInfo(PdfProcessor::bundledToolPath()).dir().filePath("qpdf30.dll"));require(!dll.open(QIODevice::WriteOnly),"verified DLL remains pinned");};f.success(f.run(f.request(PdfOperation::Inspect)),5,0);require(visited,"source pin barrier reached");}},
        {"shared real process deadline terminates child", [] {Fixture f;PdfProcessor::testTimeoutMilliseconds=1000;bool started=false;PdfProcessor::testHook=[&](auto phase,const QString&){if(phase==PdfProcessor::TestPhase::ProcessStarted){started=true;QThread::msleep(1100);}};QElapsedTimer timer;timer.start();auto result=f.run(f.request(PdfOperation::Inspect));require(started&&!result.ok&&result.message.contains("deadline")&&timer.elapsed()<5000,"bounded real child deadline");f.preserved();}},
        {"process diagnostic output bound is enforced", [] {Fixture f;PdfProcessor::testProcessOutputBytes=1;auto result=f.run(f.request(PdfOperation::Inspect));require(!result.ok&&result.message.contains("output exceeded"),"real qpdf output budget");f.preserved();}},
        {"process allocation limit is queried and fails closed at launch", [] {Fixture f;PdfProcessor::testProcessMemoryBytes=65536;bool installed=false;PdfProcessor::testHook=[&](auto phase,const QString& information){if(phase==PdfProcessor::TestPhase::ProcessLimitsInstalled){require(information=="memory=65536;processes=1;killOnClose=1","queried kernel job limits match 64 KiB test cap");installed=true;}};QElapsedTimer timer;timer.start();auto result=f.run(f.request(PdfOperation::Inspect));require(installed&&!result.ok&&!result.cancelled&&timer.elapsed()<5000&&(result.message.contains("could not start")||result.message.contains("qpdf rejected")),"configured memory limit prevents successful child execution");f.preserved();}},
        {"streamed PDF byte limit removes owned temporary", [] {Fixture f;PdfProcessor::testOutputBytes=32;auto r=f.request(PdfOperation::Rotate);r.rotation=90;auto result=f.run(r);require(!result.ok&&result.message.contains("bound")&&!QFileInfo::exists(f.output),"stream output cap");f.preserved();}},
        {"input byte limit rejects oversized synthetic PDF", [] {Fixture f;QFile file(f.source);require(file.open(QIODevice::ReadWrite)&&file.resize(PdfProcessor::MaxInputBytes+1),"large sparse synthetic PDF");file.close();auto result=f.run(f.request(PdfOperation::Inspect));require(!result.ok&&result.message.contains("bounded"),"input size bound");}},
        {"invalid page selections rotations metadata and source counts", [] {Fixture f;for(const auto& value:QStringList{"0","1;2","1,","-1",QString(4100,'1')}){auto r=f.request(PdfOperation::Extract);r.pageSpec=value;require(!f.run(r).ok,"invalid selection rejected");}auto r=f.request(PdfOperation::Extract);r.pageSpec="99";require(!f.run(r).ok,"out-of-range page rejected");r=f.request(PdfOperation::Split);r.pageSpec="1,2";require(!f.run(r).ok,"split requires integer group size");r=f.request(PdfOperation::Rotate);r.rotation=45;require(!f.run(r).ok,"invalid rotation rejected");r=f.request(PdfOperation::SetMetadata);r.metadata={{"Custom","not supported"}};require(!f.run(r).ok,"unknown metadata rejected");r.metadata={{"Title",QString(1025,'x')}};require(!f.run(r).ok,"metadata bound");r=f.request(PdfOperation::Inspect);r.sourcePaths<<f.source;require(!f.run(r).ok,"ignored extra inputs refused");f.preserved();}},
        {"every required bundle component missing fails closed", [] {const QDir dir=QFileInfo(PdfProcessor::bundledToolPath()).dir();for(auto it=detail::QpdfFiles.cbegin();it!=detail::QpdfFiles.cend();++it){const auto path=dir.filePath(it.key());const auto saved=path+".test-held";require(!QFileInfo::exists(saved)&&QFile::rename(path,saved),"hold fixture bundle component");const bool unavailable=!PdfProcessor::available();const bool restored=QFile::rename(saved,path);require(restored&&unavailable,"missing bundle component refused and restored");}require(PdfProcessor::available(),"restored complete bundle");}},
        {"DLL content tampering fails independent pinned hash", [] {const auto path=QFileInfo(PdfProcessor::bundledToolPath()).dir().filePath("qpdf30.dll");const auto original=read(path);auto altered=original;altered[altered.size()-1]^=1;put(path,altered);const bool rejected=!PdfProcessor::available();put(path,original);require(rejected&&PdfProcessor::available(),"tampered DLL refused then restored");}}
        ,{"unexpected DLL executable and text files fail exact runtime inventory", [] {const auto dir=QFileInfo(PdfProcessor::bundledToolPath()).dir();for(const auto& name:QStringList{"unexpected.dll","unexpected.exe","unexpected.txt"}){const auto path=dir.filePath(name);require(!QFileInfo::exists(path),"extra fixture path is absent");put(path,"not loaded");const bool rejected=!PdfProcessor::available();const bool removed=QFile::remove(path);require(rejected&&removed&&PdfProcessor::available(),"extra runtime entry rejected then owned fixture removed");}}}
        ,{"unexpected runtime directory fails exact inventory", [] {const auto path=QFileInfo(PdfProcessor::bundledToolPath()).dir().filePath("unexpected-directory");require(QDir().mkdir(path),"create exact owned directory fixture");const bool rejected=!PdfProcessor::available();const bool removed=QDir().rmdir(path);require(rejected&&removed&&PdfProcessor::available(),"extra directory rejected then owned empty fixture removed");}}
    };
    int failed=0;
    for(const auto& [name,test]:tests){Reset reset;try{test();std::fprintf(stderr,"PASS %s\n",name);}catch(const std::exception& e){++failed;std::fprintf(stderr,"FAIL %s: %s\n",name,e.what());}}
    std::fprintf(stderr,"PDF integration: %zu cases, %d passed, %d failed\n",tests.size(),int(tests.size())-failed,failed);
    return failed?1:0;
}
