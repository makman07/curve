/*
 * Project: curve
 * Created Date: Tue Apr 02 2019
 * Author: xuchaojie
 * Copyright (c) 2018 netease
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "src/snapshotcloneserver/clone/clone_core.h"
#include "src/snapshotcloneserver/clone/clone_task.h"
#include "src/snapshotcloneserver/common/define.h"

#include "test/snapshotcloneserver/mock_snapshot_server.h"

using ::testing::Return;
using ::testing::_;
using ::testing::AnyOf;
using ::testing::AllOf;
using ::testing::SetArgPointee;
using ::testing::Invoke;
using ::testing::DoAll;

namespace curve {
namespace snapshotcloneserver {

class TestCloneCoreImpl : public ::testing::Test {
 public:
    TestCloneCoreImpl() {}
    virtual ~TestCloneCoreImpl() {}

    virtual void SetUp() {
        snapshotRef_ =
            std::make_shared<SnapshotReference>();
        client_ = std::make_shared<MockCurveFsClient>();
        metaStore_ = std::make_shared<MockSnapshotCloneMetaStore>();
        dataStore_ = std::make_shared<MockSnapshotDataStore>();
        option.cloneTempDir = "/clone";
        option.cloneChunkSplitSize = 1024 * 1024;
        core_ = std::make_shared<CloneCoreImpl>(client_,
            metaStore_,
            dataStore_,
            snapshotRef_,
            option);
    }

    virtual void TearDown() {
        client_ = nullptr;
        metaStore_ = nullptr;
        dataStore_ = nullptr;
        core_ = nullptr;
    }

 protected:
    // 辅助mock函数
    void MockBuildFileInfoFromSnapshotSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockBuildFileInfoFromFileSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCreateCloneFileSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCloneMetaSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCreateCloneChunkSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCompleteCloneMetaSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockRecoverChunkSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockRenameCloneFileSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCompleteCloneFileSuccess(
        std::shared_ptr<CloneTaskInfo> task);

    void MockBuildFileInfoFromSnapshotFail(
        std::shared_ptr<CloneTaskInfo> task);

    void MockBuildFileInfoFromFileFail(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCreateCloneFileFail(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCloneMetaFail(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCreateCloneChunkFail(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCompleteCloneMetaFail(
        std::shared_ptr<CloneTaskInfo> task);

    void MockRecoverChunkFail(
        std::shared_ptr<CloneTaskInfo> task);

    void MockRenameCloneFileFail(
        std::shared_ptr<CloneTaskInfo> task);

    void MockCompleteCloneFileFail(
        std::shared_ptr<CloneTaskInfo> task);

 protected:
    std::shared_ptr<CloneCoreImpl> core_;
    std::shared_ptr<MockCurveFsClient> client_;
    std::shared_ptr<MockSnapshotCloneMetaStore> metaStore_;
    std::shared_ptr<MockSnapshotDataStore> dataStore_;
    std::shared_ptr<SnapshotReference> snapshotRef_;
    SnapshotCloneServerOptions option;
};

TEST_F(TestCloneCoreImpl, TestClonePreForSnapSuccess) {
    const UUID &source = "id1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;

    std::vector<CloneInfo> list;
    CloneInfo cinfo;
    cinfo.SetSrc(source);
    cinfo.SetStatus(CloneStatus::done);
    list.push_back(cinfo);

    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(DoAll(
                    SetArgPointee<0>(list),
                    Return(true)));

    SnapshotInfo snap("id1", "user1", "file1", "snap1");
    snap.SetStatus(Status::done);
    EXPECT_CALL(*metaStore_, GetSnapshotInfo(source, _))
        .WillOnce(DoAll(
                SetArgPointee<1>(snap),
                Return(kErrCodeSuccess)));

    EXPECT_CALL(*metaStore_, AddCloneInfo(_))
        .WillOnce(Return(kErrCodeSuccess));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeSuccess, ret);
}

TEST_F(TestCloneCoreImpl, TestClonePreForFileSuccess) {
    const UUID &source = "fi1e1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;

    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(Return(true));
    EXPECT_CALL(*metaStore_, GetSnapshotInfo(source, _))
        .WillOnce(Return(kErrCodeInternalError));

    EXPECT_CALL(*client_, GetFileInfo(source, user, _))
        .WillOnce(Return(LIBCURVE_ERROR::OK));

    EXPECT_CALL(*metaStore_, AddCloneInfo(_))
        .WillOnce(Return(kErrCodeSuccess));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeSuccess, ret);
}

TEST_F(TestCloneCoreImpl, TestClonePreForSnapInvalidSnapshot) {
    const UUID &source = "fi1e1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;
    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(Return(true));

    SnapshotInfo snap("id1", "user2", "file1", "snap1");
    snap.SetStatus(Status::pending);
    EXPECT_CALL(*metaStore_, GetSnapshotInfo(source, _))
        .WillOnce(DoAll(
                SetArgPointee<1>(snap),
                Return(kErrCodeSuccess)));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeInvalidSnapshot, ret);
}

TEST_F(TestCloneCoreImpl, TestClonePreForSnapInvalidUser) {
    const UUID &source = "fi1e1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;
    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(Return(true));

    SnapshotInfo snap("id1", "user2", "file1", "snap1");
    snap.SetStatus(Status::done);
    EXPECT_CALL(*metaStore_, GetSnapshotInfo(source, _))
        .WillOnce(DoAll(
                SetArgPointee<1>(snap),
                Return(kErrCodeSuccess)));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeInvalidUser, ret);
}

TEST_F(TestCloneCoreImpl, TestClonePreAddCloneInfoFail) {
    const UUID &source = "id1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;
    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(Return(true));

    SnapshotInfo snap("id1", "user1", "file1", "snap1");
    snap.SetStatus(Status::done);
    EXPECT_CALL(*metaStore_, GetSnapshotInfo(source, _))
        .WillOnce(DoAll(
                SetArgPointee<1>(snap),
                Return(kErrCodeSuccess)));

    EXPECT_CALL(*metaStore_, AddCloneInfo(_))
        .WillOnce(Return(kErrCodeInternalError));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeInternalError, ret);
}

TEST_F(TestCloneCoreImpl, TestClonePreForFileNotExist) {
    const UUID &source = "fi1e1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;
    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(Return(true));

    EXPECT_CALL(*metaStore_, GetSnapshotInfo(source, _))
        .WillOnce(Return(kErrCodeInternalError));

    EXPECT_CALL(*client_, GetFileInfo(source, user, _))
        .WillOnce(Return(LIBCURVE_ERROR::NOTEXIST));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeFileNotExist, ret);
}

TEST_F(TestCloneCoreImpl, TestClonePreForFileAUTHFAIL) {
    const UUID &source = "fi1e1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;
    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(Return(true));

    EXPECT_CALL(*metaStore_, GetSnapshotInfo(source, _))
        .WillOnce(Return(kErrCodeInternalError));

    EXPECT_CALL(*client_, GetFileInfo(source, user, _))
        .WillOnce(Return(LIBCURVE_ERROR::AUTHFAIL));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeInvalidUser, ret);
}

TEST_F(TestCloneCoreImpl, TestClonePreForFileFail) {
    const UUID &source = "fi1e1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;
    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(Return(true));

    EXPECT_CALL(*metaStore_, GetSnapshotInfo(source, _))
        .WillOnce(Return(kErrCodeInternalError));

    EXPECT_CALL(*client_, GetFileInfo(source, user, _))
        .WillOnce(Return(LIBCURVE_ERROR::FAILED));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeInternalError, ret);
}

TEST_F(TestCloneCoreImpl, TestClonePreFailHasError) {
    const UUID &source = "fi1e1";
    const std::string user = "user1";
    const std::string destination = "destination1";
    bool lazyFlag = true;
    CloneInfo cloneInfoOut;

    std::vector<CloneInfo> list;
    CloneInfo cinfo;
    cinfo.SetDest(destination);
    cinfo.SetStatus(CloneStatus::error);
    list.push_back(cinfo);

    EXPECT_CALL(*metaStore_, GetCloneInfoList(_))
        .WillOnce(DoAll(
                    SetArgPointee<0>(list),
                    Return(true)));

    int ret = core_->CloneOrRecoverPre(
        source, user, destination, lazyFlag,
        CloneTaskType::kClone, &cloneInfoOut);
    ASSERT_EQ(kErrCodeSnapshotCannotCreateWhenError, ret);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskSuccessForCloneBySnapshot) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkSuccess(task);
    MockCompleteCloneMetaSuccess(task);
    MockRenameCloneFileSuccess(task);
    MockRecoverChunkSuccess(task);
    MockCompleteCloneFileSuccess(task);
    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl,
    HandleCloneOrRecoverTaskSuccessForCloneBySnapshotNotLazy) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, false);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkSuccess(task);
    MockCompleteCloneMetaSuccess(task);
    MockRecoverChunkSuccess(task);
    MockCompleteCloneFileSuccess(task);
    MockRenameCloneFileSuccess(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl,
    HandleCloneOrRecoverTaskFailOnBuildFileInfoFromSnapshot) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl,
    HandleCloneOrRecoverTaskFailOnGetSnapshotInfo) {
    CloneInfo cinfo("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    cinfo.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(cinfo);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    UUID uuid = "uuid1";
    const std::string user = "user1";
    const std::string fileName = "file1";
    const std::string desc = "snap1";
    uint64_t seqnum = 100;
    uint32_t chunksize = 1024 * 1024;
    uint64_t segmentsize = 2 * chunksize;
    uint64_t filelength = 2 * segmentsize;
    uint64_t time = 100;
    Status status = Status::done;
    SnapshotInfo info(uuid, user, fileName, desc,
        seqnum, chunksize, segmentsize, filelength, time, status);

    EXPECT_CALL(*metaStore_, GetSnapshotInfo(_, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<1>(info),
                Return(kErrCodeInternalError)));

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskSuccessForRecoverBySnapshot) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::recovering);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkSuccess(task);
    MockCompleteCloneMetaSuccess(task);
    MockRenameCloneFileSuccess(task);
    MockRecoverChunkSuccess(task);
    MockCompleteCloneFileSuccess(task);
    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl,
    HandleCloneOrRecoverTaskFailOnCreateCloneFile) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskFailOnCloneMeta) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskFailOnCreateCloneChunk) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskFailOnCompleteCloneMeta) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkSuccess(task);
    MockCompleteCloneMetaFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskFileOnRenameCloneFile) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkSuccess(task);
    MockCompleteCloneMetaSuccess(task);
    MockRenameCloneFileFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskFileOnRecoverChunk) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkSuccess(task);
    MockCompleteCloneMetaSuccess(task);
    MockRenameCloneFileSuccess(task);
    MockRecoverChunkFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskFailOnCompleteCloneFail) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkSuccess(task);
    MockCompleteCloneMetaSuccess(task);
    MockRenameCloneFileSuccess(task);
    MockRecoverChunkSuccess(task);
    MockCompleteCloneFileFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, HandleCloneOrRecoverTaskSuccessForCloneByFile) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kFile, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromFileSuccess(task);
    MockCreateCloneFileSuccess(task);
    MockCloneMetaSuccess(task);
    MockCreateCloneChunkSuccess(task);
    MockCompleteCloneMetaSuccess(task);
    MockRenameCloneFileSuccess(task);
    MockRecoverChunkSuccess(task);
    MockCompleteCloneFileSuccess(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl,
    HandleCloneOrRecoverTaskForCloneByFileFailOnBuildFileInfoFromFile) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kFile, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromFileFail(task);

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl,
    HandleCloneOrRecoverTaskForCloneByFileFailOnInvalidSegmentSize) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kFile, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    FInfo fInfo;
    fInfo.id = 100;
    fInfo.chunksize = 1024 * 1024;
    fInfo.segmentsize = 0;
    fInfo.length = 2 * fInfo.segmentsize;
    fInfo.seqnum = 100;
    fInfo.owner = "user1";
    fInfo.filename = "file1";
    EXPECT_CALL(*client_, GetFileInfo(_, _, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<2>(fInfo),
                Return(LIBCURVE_ERROR::OK)));

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl,
    HandleCloneOrRecoverTaskForCloneByFileFailOnInvalidFileLen) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kFile, true);
    info.SetStatus(CloneStatus::cloning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    FInfo fInfo;
    fInfo.id = 100;
    fInfo.chunksize = 1024 * 1024;
    fInfo.segmentsize = 2 * fInfo.chunksize;
    fInfo.length = 1;
    fInfo.seqnum = 100;
    fInfo.owner = "user1";
    fInfo.filename = "file1";
    EXPECT_CALL(*client_, GetFileInfo(_, _, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<2>(fInfo),
                Return(LIBCURVE_ERROR::OK)));

    core_->HandleCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl,
    HandleCloneOrRecoverTaskStepUnknown) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cloning);
    info.SetNextStep(static_cast<CloneStep>(8));
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillRepeatedly(Return(kErrCodeSuccess));

    MockBuildFileInfoFromSnapshotSuccess(task);
    MockCloneMetaSuccess(task);

    core_->HandleCloneOrRecoverTask(task);
}

void TestCloneCoreImpl::MockBuildFileInfoFromSnapshotSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    UUID uuid = "uuid1";
    const std::string user = "user1";
    const std::string fileName = "file1";
    const std::string desc = "snap1";
    uint64_t seqnum = 100;
    uint32_t chunksize = 1024 * 1024;
    uint64_t segmentsize = 2 * chunksize;
    uint64_t filelength = 2 * segmentsize;
    uint64_t time = 100;
    Status status = Status::done;
    SnapshotInfo info(uuid, user, fileName, desc,
        seqnum, chunksize, segmentsize, filelength, time, status);

    EXPECT_CALL(*metaStore_, GetSnapshotInfo(_, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<1>(info),
                Return(kErrCodeSuccess)));

    if (CloneTaskType::kRecover == task->GetCloneInfo().GetTaskType()) {
        FInfo fInfo;
        fInfo.id = 100;
        EXPECT_CALL(*client_, GetFileInfo(_, _, _))
            .WillRepeatedly(DoAll(
                    SetArgPointee<2>(fInfo),
                    Return(LIBCURVE_ERROR::OK)));
    }

    ChunkIndexData snapMeta;
    ChunkDataName chunk1("file1", 1, 0);
    ChunkDataName chunk2("file1", 1, 1);
    ChunkDataName chunk3("file1", 1, 2);
    ChunkDataName chunk4("file1", 1, 3);
    snapMeta.PutChunkDataName(chunk1);
    snapMeta.PutChunkDataName(chunk2);
    snapMeta.PutChunkDataName(chunk3);
    snapMeta.PutChunkDataName(chunk4);

    EXPECT_CALL(*dataStore_, GetChunkIndexData(_, _))
        .WillOnce(DoAll(
                    SetArgPointee<1>(snapMeta),
                    Return(kErrCodeSuccess)));

    FInfo fInfo;
    fInfo.id = 100;
    EXPECT_CALL(*client_, GetFileInfo(_, _, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<2>(fInfo),
                Return(LIBCURVE_ERROR::OK)));
}

void TestCloneCoreImpl::MockBuildFileInfoFromFileSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    FInfo fInfo;
    fInfo.id = 100;
    fInfo.chunksize = 1024 * 1024;
    fInfo.segmentsize = 2 * fInfo.chunksize;
    fInfo.length = 2 * fInfo.segmentsize;
    fInfo.seqnum = 100;
    fInfo.owner = "user1";
    fInfo.filename = "file1";
    EXPECT_CALL(*client_, GetFileInfo(_, _, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<2>(fInfo),
                Return(LIBCURVE_ERROR::OK)));
}

void TestCloneCoreImpl::MockCreateCloneFileSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    FInfo fInfoOut;
    fInfoOut.id = 100;
    EXPECT_CALL(*client_, CreateCloneFile(_, _, _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<5>(fInfoOut),
                Return(LIBCURVE_ERROR::OK)));
}

void TestCloneCoreImpl::MockCloneMetaSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, GetOrAllocateSegmentInfo(_, _, _, _, _))
        .WillRepeatedly(Return(LIBCURVE_ERROR::OK));
}

void TestCloneCoreImpl::MockCreateCloneChunkSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, CreateCloneChunk(_, _, _, _, _))
        .WillRepeatedly(Return(LIBCURVE_ERROR::OK));
}

void TestCloneCoreImpl::MockCompleteCloneMetaSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, CompleteCloneMeta(_, _))
        .WillOnce(Return(LIBCURVE_ERROR::OK));
}

void TestCloneCoreImpl::MockRecoverChunkSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, RecoverChunk(_, _, _))
        .WillRepeatedly(Return(LIBCURVE_ERROR::OK));
}

void TestCloneCoreImpl::MockRenameCloneFileSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, RenameCloneFile(_, _, _, _, _))
        .WillOnce(Return(LIBCURVE_ERROR::OK));
}

void TestCloneCoreImpl::MockCompleteCloneFileSuccess(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, CompleteCloneFile(_, _))
        .WillOnce(Return(LIBCURVE_ERROR::OK));
}

void TestCloneCoreImpl::MockBuildFileInfoFromSnapshotFail(
    std::shared_ptr<CloneTaskInfo> task) {
    UUID uuid = "uuid1";
    const std::string user = "user1";
    const std::string fileName = "file1";
    const std::string desc = "snap1";
    uint64_t seqnum = 100;
    uint32_t chunksize = 1024 * 1024;
    uint64_t segmentsize = 2 * chunksize;
    uint64_t filelength = 2 * segmentsize;
    uint64_t time = 100;
    Status status = Status::done;
    SnapshotInfo info(uuid, user, fileName, desc,
        seqnum, chunksize, segmentsize, filelength, time, status);

    EXPECT_CALL(*metaStore_, GetSnapshotInfo(_, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<1>(info),
                Return(kErrCodeSuccess)));

    if (CloneTaskType::kRecover == task->GetCloneInfo().GetTaskType()) {
        FInfo fInfo;
        fInfo.id = 100;
        EXPECT_CALL(*client_, GetFileInfo(_, _, _))
            .WillRepeatedly(DoAll(
                    SetArgPointee<2>(fInfo),
                    Return(LIBCURVE_ERROR::OK)));
    }

    EXPECT_CALL(*dataStore_, GetChunkIndexData(_, _))
        .WillOnce(Return(kErrCodeInternalError));

    FInfo fInfo;
    fInfo.id = 100;
    EXPECT_CALL(*client_, GetFileInfo(_, _, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<2>(fInfo),
                Return(LIBCURVE_ERROR::FAILED)));
}

void TestCloneCoreImpl::MockBuildFileInfoFromFileFail(
    std::shared_ptr<CloneTaskInfo> task) {
    FInfo fInfo;
    fInfo.id = 100;
    fInfo.chunksize = 1024 * 1024;
    fInfo.segmentsize = 2 * fInfo.chunksize;
    fInfo.length = 2 * fInfo.segmentsize;
    fInfo.seqnum = 100;
    fInfo.owner = "user1";
    fInfo.filename = "file1";
    EXPECT_CALL(*client_, GetFileInfo(_, _, _))
        .WillRepeatedly(DoAll(
                SetArgPointee<2>(fInfo),
                Return(LIBCURVE_ERROR::FAILED)));
}

void TestCloneCoreImpl::MockCreateCloneFileFail(
    std::shared_ptr<CloneTaskInfo> task) {
    FInfo fInfoOut;
    fInfoOut.id = 100;
    EXPECT_CALL(*client_, CreateCloneFile(_, _, _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<5>(fInfoOut),
            Return(LIBCURVE_ERROR::FAILED)));
}

void TestCloneCoreImpl::MockCloneMetaFail(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, GetOrAllocateSegmentInfo(_, _, _, _, _))
        .WillRepeatedly(Return(LIBCURVE_ERROR::FAILED));
}

void TestCloneCoreImpl::MockCreateCloneChunkFail(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, CreateCloneChunk(_, _, _, _, _))
        .WillRepeatedly(Return(LIBCURVE_ERROR::FAILED));
}

void TestCloneCoreImpl::MockCompleteCloneMetaFail(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, CompleteCloneMeta(_, _))
        .WillOnce(Return(LIBCURVE_ERROR::FAILED));
}

void TestCloneCoreImpl::MockRecoverChunkFail(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, RecoverChunk(_, _, _))
        .WillRepeatedly(Return(LIBCURVE_ERROR::FAILED));
}

void TestCloneCoreImpl::MockRenameCloneFileFail(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, RenameCloneFile(_, _, _, _, _))
        .WillOnce(Return(LIBCURVE_ERROR::FAILED));
}

void TestCloneCoreImpl::MockCompleteCloneFileFail(
    std::shared_ptr<CloneTaskInfo> task) {
    EXPECT_CALL(*client_, CompleteCloneFile(_, _))
        .WillOnce(Return(LIBCURVE_ERROR::FAILED));
}

TEST_F(TestCloneCoreImpl, TestCleanOrRecoverTaskPreSuccess) {
    const TaskIdType &taskId = "id1";
    const std::string user = "user1";
    CloneInfo cloneInfoOut;

    CloneInfo cinfo;
    cinfo.SetTaskId(taskId);
    cinfo.SetUser(user);
    cinfo.SetStatus(CloneStatus::error);

    EXPECT_CALL(*metaStore_, GetCloneInfo(_, _))
        .WillOnce(DoAll(
                  SetArgPointee<1>(cinfo),
                  Return(0)));

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillOnce(Return(kErrCodeSuccess));

    int ret = core_->CleanCloneOrRecoverTaskPre(
        user, taskId, &cloneInfoOut);
    ASSERT_EQ(kErrCodeSuccess, ret);
}

TEST_F(TestCloneCoreImpl, TestCleanOrRecoverTaskPreGetCloneInfoFail) {
    const TaskIdType &taskId = "id1";
    const std::string user = "user1";
    CloneInfo cloneInfoOut;

    CloneInfo cinfo;
    cinfo.SetTaskId(taskId);
    cinfo.SetUser(user);
    cinfo.SetStatus(CloneStatus::error);

    EXPECT_CALL(*metaStore_, GetCloneInfo(_, _))
        .WillOnce(DoAll(
                  SetArgPointee<1>(cinfo),
                  Return(-1)));

    int ret = core_->CleanCloneOrRecoverTaskPre(
        user, taskId, &cloneInfoOut);
    ASSERT_EQ(kErrCodeFileNotExist, ret);
}

TEST_F(TestCloneCoreImpl, TestCleanOrRecoverTaskPreInvalidUser) {
    const TaskIdType &taskId = "id1";
    const std::string user = "user1";
    CloneInfo cloneInfoOut;

    CloneInfo cinfo;
    cinfo.SetTaskId(taskId);
    cinfo.SetUser("user2");
    cinfo.SetStatus(CloneStatus::error);

    EXPECT_CALL(*metaStore_, GetCloneInfo(_, _))
        .WillOnce(DoAll(
                  SetArgPointee<1>(cinfo),
                  Return(0)));

    int ret = core_->CleanCloneOrRecoverTaskPre(
        user, taskId, &cloneInfoOut);
    ASSERT_EQ(kErrCodeInvalidUser, ret);
}

TEST_F(TestCloneCoreImpl, TestCleanOrRecoverTaskPreCannotCleanNotError) {
    const TaskIdType &taskId = "id1";
    const std::string user = "user1";
    CloneInfo cloneInfoOut;

    CloneInfo cinfo;
    cinfo.SetTaskId(taskId);
    cinfo.SetUser(user);
    cinfo.SetStatus(CloneStatus::done);

    EXPECT_CALL(*metaStore_, GetCloneInfo(_, _))
        .WillOnce(DoAll(
                  SetArgPointee<1>(cinfo),
                  Return(0)));

    int ret = core_->CleanCloneOrRecoverTaskPre(
        user, taskId, &cloneInfoOut);
    ASSERT_EQ(kErrCodeCannotCleanCloneNotError, ret);
}

TEST_F(TestCloneCoreImpl, TestCleanOrRecoverTaskPreTaskExist) {
    const TaskIdType &taskId = "id1";
    const std::string user = "user1";
    CloneInfo cloneInfoOut;

    CloneInfo cinfo;
    cinfo.SetTaskId(taskId);
    cinfo.SetUser(user);
    cinfo.SetStatus(CloneStatus::cleaning);

    EXPECT_CALL(*metaStore_, GetCloneInfo(_, _))
        .WillOnce(DoAll(
                  SetArgPointee<1>(cinfo),
                  Return(0)));

    int ret = core_->CleanCloneOrRecoverTaskPre(
        user, taskId, &cloneInfoOut);
    ASSERT_EQ(kErrCodeTaskExist, ret);
}

TEST_F(TestCloneCoreImpl, TestCleanOrRecoverTaskPreUpdateCloneInfoFail) {
    const TaskIdType &taskId = "id1";
    const std::string user = "user1";
    CloneInfo cloneInfoOut;

    CloneInfo cinfo;
    cinfo.SetTaskId(taskId);
    cinfo.SetUser(user);
    cinfo.SetStatus(CloneStatus::error);

    EXPECT_CALL(*metaStore_, GetCloneInfo(_, _))
        .WillOnce(DoAll(
                  SetArgPointee<1>(cinfo),
                  Return(0)));

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillOnce(Return(kErrCodeInternalError));

    int ret = core_->CleanCloneOrRecoverTaskPre(
        user, taskId, &cloneInfoOut);
    ASSERT_EQ(kErrCodeInternalError, ret);
}

TEST_F(TestCloneCoreImpl, TestHandleCleanCloneOrRecoverTaskSuccess) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, false);
    info.SetStatus(CloneStatus::cleaning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*client_, DeleteFile(_, _, _))
        .Times(2)
        .WillOnce(Return(LIBCURVE_ERROR::OK))
        .WillOnce(Return(LIBCURVE_ERROR::NOTEXIST));

    EXPECT_CALL(*metaStore_, DeleteCloneInfo(_))
        .WillOnce(Return(0));
    core_->HandleCleanCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, TestHandleCleanCloneOrRecoverTaskSuccess2) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, false);
    info.SetStatus(CloneStatus::cleaning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*client_, DeleteFile(_, _, _))
        .Times(2)
        .WillOnce(Return(LIBCURVE_ERROR::NOTEXIST))
        .WillOnce(Return(LIBCURVE_ERROR::OK));

    EXPECT_CALL(*metaStore_, DeleteCloneInfo(_))
        .WillOnce(Return(0));
    core_->HandleCleanCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, TestHandleCleanCloneOrRecoverTaskLazySuccess) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, true);
    info.SetStatus(CloneStatus::cleaning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*client_, DeleteFile(_, _, _))
        .Times(1)
        .WillOnce(Return(LIBCURVE_ERROR::OK));

    EXPECT_CALL(*metaStore_, DeleteCloneInfo(_))
        .WillOnce(Return(0));
    core_->HandleCleanCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, TestHandleCleanCloneOrRecoverTaskFail1) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, false);
    info.SetStatus(CloneStatus::cleaning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*client_, DeleteFile(_, _, _))
        .Times(1)
        .WillOnce(Return(LIBCURVE_ERROR::FAILED));

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillOnce(Return(0));

    core_->HandleCleanCloneOrRecoverTask(task);
}

TEST_F(TestCloneCoreImpl, TestHandleCleanCloneOrRecoverTaskFail2) {
    CloneInfo info("id1", "user1", CloneTaskType::kClone,
    "snapid1", "file1", CloneFileType::kSnapshot, false);
    info.SetStatus(CloneStatus::cleaning);
    std::shared_ptr<CloneTaskInfo> task =
        std::make_shared<CloneTaskInfo>(info);

    EXPECT_CALL(*client_, DeleteFile(_, _, _))
        .Times(2)
        .WillOnce(Return(LIBCURVE_ERROR::NOTEXIST))
        .WillOnce(Return(LIBCURVE_ERROR::FAILED));

    EXPECT_CALL(*metaStore_, UpdateCloneInfo(_))
        .WillOnce(Return(0));

    core_->HandleCleanCloneOrRecoverTask(task);
}

}  // namespace snapshotcloneserver
}  // namespace curve
