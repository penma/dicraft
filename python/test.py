import dicomprint

wat = dicomprint.load_dicom_dir("../dicom-files/01uk")
bin = wat.threshold(1700)
bin._write_pnm("wat/")

